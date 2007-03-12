require 'test/unit'
require File.dirname(__FILE__)+"/../lib/ruby-orbit"

class FixedLengthStruct
  attr_accessor :a
  def initialize(value = nil)
    @a = value || 4
  end
end

VariableLengthStruct = FixedLengthStruct


class TestFactory < Test::Unit::TestCase
  File.open("/Users/max/Sites/ORBit2/ORBit2-2.14.7/test/everything/iorfile") do |f|
    @server = ORBit2::CorbaObject.from_ior(f.read)
  end
  
  def self.server
    @server
  end
  
  def setup
    @server = self.class.server
  end
  
  def test_basic
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @basicServer = @server.getBasicServer
    _in = 0x12345678
    inout = ORBit2::Long.new 0x34567812
    out = ORBit2::Long.new 0  
    assert_equal -1430532899, @basicServer.opLong(_in, inout, out)
    assert_equal 1450709556, inout.to_i
    assert_equal 2014458966, out.to_i
  end
  
  def test_any
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @anyServer = @server.getAnyServer
    assert_equal ["opAnyStrSeq", "opAnyLong", "opAnyString", "opAnyStruct", "opTypeCode"], @anyServer.corba_methods
    assert_equal "any opAnyLong(any inArg, inout any inoutArg, out any outArg);", @anyServer.describe_method(:opAnyLong)

    _in = 0x12345678
    inout = ORBit2::Long.new 0x34567812
    out = ORBit2::Long.new 0
    assert_equal -1430532899, @anyServer.opAnyLong(_in, inout, out)
    assert_equal 1450709556, inout.to_i
    assert_equal 2014458966, out.to_i

    _in = "In string"
    inout = "Inout in string";
    out = ""
    assert_equal "Retn String", @anyServer.opAnyString(_in, inout, out)
    assert_equal "Inout out string", inout
    assert_equal "Out string", out
  end
  
  def test_struct
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @structServer = @server.getStructServer
    assert_equal ["opFixed", "opVariable", "opCompound", "opAlignHole", "opObjectStruct", "opStructAny"], @structServer.corba_methods
    _in = FixedLengthStruct.new(0x1234)
    inout = FixedLengthStruct.new(0x3456)
    out = FixedLengthStruct.new
    retn = @structServer.opFixed(_in, inout, out)
    assert_equal 0xAABB, retn.a
    assert_equal 0x5678, inout.a
    assert_equal 0x7812, out.a


    _in = VariableLengthStruct.new("In string")
    inout = VariableLengthStruct.new("Inout in string")
    out = VariableLengthStruct.new("")
    retn = @structServer.opVariable(_in, inout, out)
    assert_equal "Retn String", retn.a
    assert_equal "Inout out string", inout.a
    assert_equal "Out string", out.a
  end
end