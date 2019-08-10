require 'pp'
#require_relative 'graph'
require 'stringio'

class State

  A_STATES = [
      [:c,:c],
      [:c,:d],
      [:d,:c],
      [:d,:d]
  ].map {|x| x.freeze}.freeze

  B_STATES = Marshal.load( Marshal.dump(A_STATES) ).freeze

  ALL_STATES = A_STATES.product(B_STATES).map {|a,b| (a+b).freeze }.freeze

  def self.valid?(state)
    ALL_STATES.include?( state )
  end

  def self.index( state )
    ALL_STATES.index( state )
  end

  def self.make_from_id( id )
    raise "invalid arg: #{id}" if id < 0 or id > 15
    a = (0..3).reverse_each.map do |i|
      id[i] == 1 ? :d : :c
    end
    self.new(*a)
  end

  def self.make_from_str(s)
    raise unless s =~ /\A[cd]{4}\z/
    self.new( *s.each_char.map(&:to_sym) )
  end

  def self.c(str)
    str
      .gsub('c',"\e[42m\e[30mc\e[0m")
      .gsub('d',"\e[45m\e[30md\e[0m")
  end

  attr_reader :a_2,:a_1,:b_2,:b_1

  def initialize(a_2,a_1,b_2,b_1)
    @a_2 = a_2
    @a_1 = a_1
    @b_2 = b_2
    @b_1 = b_1
    unless [@a_2,@a_1,@b_2,@b_1].all? {|a| a == :d or a == :c }
      raise "invalid state"
    end
  end

  def to_a
    [@a_2,@a_1,@b_2,@b_1]
  end

  def to_s
    to_a.join('')
  end

  def inspect
    self.class.c(to_s)
  end

  def ==(other)
    self.to_id == other.to_id
  end

  def to_id
    to_a.map {|x| x==:d ? '1' : '0' }.join.to_i(2)
  end
  alias :to_i :to_id

  def next_state(act_a,act_b)
    self.class.new(@a_1,act_a,@b_1,act_b)
  end

  def prev_state(act_a,act_b)
    self.class.new(act_a,@a_2,act_b,@b_2)
  end

  def possible_prev_states
    [:c,:d].map do |a|
      [:c,:d].map do |b|
        prev_state(a,b)
      end
    end.flatten
  end

  def swap
    self.class.new(@b_2,@b_1,@a_2,@a_1)
  end

  def relative_payoff
    a = @a_1
    b = @b_1

    if a == b
      return 0
    elsif a == :c and b == :d
      return -1
    elsif a == :d and b == :c
      return 1
    else
      raise "must not happen"
    end
  end
end


class Strategy

  A = [:c,:d]

  def initialize( actions )
    raise unless actions.size == 16
    raise unless actions.all? {|a| self.class::A.include?(a) }
    @strategy = actions.dup
  end

  def to_s
    @strategy.join('')
  end

  def to_i
    ans = 0
    State::ALL_STATES.size.times do |idx|
      i = (@strategy[idx] == :c ? 0 : 1)
      ans += (i << idx)
    end
    ans
  end

  def inspect
    sio = StringIO.new
    sio.puts "#{to_s} #{to_i}"
    State::ALL_STATES.each_with_index do |stat,idx|
      sio.print "#{@strategy[idx]}|#{stat.map(&:to_s).join}\t"
      sio.print "\n" if idx % 8 == 7
    end
    State.c(sio.string)
  end

  def show_actions(io)
    State::ALL_STATES.each_with_index do |stat,idx|
      io.print "#{@strategy[idx]}|#{stat.map(&:to_s).join}\t"
      io.print "\n" if idx % 8 == 7
    end
  end

  def self.make_from_str( bits )
    raise "invalid format" unless bits =~ /\A[cd]{16}\z/
    actions = bits.each_char.map(&:to_sym)
    self.new( actions )
  end

  def action( state )
    s =
      case state
      when State
        state
      when Array
        State.new(*state)
      when String
        State.make_from_str(state)
      else
        raise "invalid input"
      end
    @strategy[s.to_i]
  end

  def valid?
    @strategy.all? {|a| self.class::A.include?(a) }
  end

  def set( state, act )
    raise "#{self.class::A.inspect}" unless self.class::A.include?(act)
    s =
      case state
      when State
        state
      when Array
        State.new(*state)
      when String
        State.make_from_str(state)
      else
        raise "invalid input"
      end
    @strategy[s.to_i] = act
  end

  def possible_next_states(current)
    act_a = action(current)
    n1 = current.next_state(act_a,:c)
    n2 = current.next_state(act_a,:d)
    [n1,n2]
  end

  def next_state_with_self(current)
    act_a = action(current)
    act_b = action(current.swap)
    current.next_state(act_a,act_b)
  end

  def transition_graph
    g = DirectedGraph.new(16)
    16.times do |i|
      s = State.make_from_id(i)
      next_ss = possible_next_states(s)
      next_ss.each do |n|
        g.add_link(i,n.to_i)
      end
    end
    g
  end

  def transition_graph_with_self
    g = DirectedGraph.new(16)
    16.times do |i|
      s = State.make_from_id(i)
      n = next_state_with_self(s)
      g.add_link( i, n.to_i )
    end
    g
  end

  def self.node_attributes
    node_attributes = {}
    64.times do |i|
      s = State.make_from_id(i)
      node_attributes[i] = {}
      node_attributes[i][:label] = "#{i}_#{s}"
    end
    node_attributes
  end

  def defensible?
    g = weighted_transition_graph
    !(g.has_negative_cycle?)
  end

  def weighted_transition_graph
    g = DirectedWeightedGraph.new(16)
    16.times do |i|
      s = State.make_from_id(i)
      ns = possible_next_states(s)
      ns.each do |n|
        j = n.to_id
        g.add_link(i,j,n.relative_payoff)
      end
    end
    g
  end
end

if __FILE__ == $0 and ARGV.size > 0
  ARGV.each do |s|
    pp Strategy.make_from_str(s)
  end
end

