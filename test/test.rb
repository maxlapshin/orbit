require 'test/unit'
require File.dirname(__FILE__)+"/../lib/ruby-orbit"

FixedLengthStruct = Struct.new :a
VariableLengthStruct = Struct.new :a
CompoundStruct = Struct.new :a
AlignHoleInnerStruct = Struct.new :a, :b
AlignHoleStruct = Struct.new :a, :b
StructAny = Struct.new :a, :b
ObjectStruct = Struct.new :serv

LONG_IN = ORBit2::Long.new(305419896) #0x12345678
LONG_INOUT_IN = ORBit2::Long.new(878082066) #0x34567812
LONG_INOUT_OUT = ORBit2::Long.new(1450709556) #0x56781234
LONG_OUT = ORBit2::Long.new(2014458966) #0x78123456
LONG_RETN = ORBit2::Long.new(-1430532899) #0xAABBCCDD

SEQ_STRING_IN = [ "in1","in2","in3","in4" ]
SEQ_STRING_OUT = ["out1","out2","out3","out4"]
SEQ_STRING_INOUT_IN = ["inout1","inout2","inout3","inout4"]
SEQ_STRING_INOUT_OUT = ["inout21","inout22","inout23","inout24"] 
SEQ_STRING_RETN = ["retn1","retn2","retn3","retn4"]

SEQ_OCTET_IN = [1, 3, 5, 7]
SEQ_OCTET_INOUT_IN = [1, 15, 8, 0]
SEQ_OCTET_INOUT_OUT = [73, 128, 173, 15]
SEQ_OCTET_OUT = [2, 7, 9, 255]
SEQ_OCTET_RETN = [1, 3, 5, 7]


module Orbit
  module Test
    class TestException < StandardError
    end
    
    class TestFactory < ORBit2::CorbaObject
    end
    
    class BasicServer < ORBit2::CorbaObject
    end
  end
end


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
  
  def test_long
    assert_equal ORBit2::Long.new(4), 4
    assert_equal 4, ORBit2::Long.new(4)
  end
  
  def test_basic
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @basicServer = @server.getBasicServer
    assert_equal ["_get_foo", "_set_foo", "_get_bah", "opString", "opLong", "opLongLong", 
      "opFloat", "opDouble", "opLongDouble", "opEnum", "opException", "opOneWay", 
      "noImplement", "testLargeStringSeq", "getObjectCount", "getObject", "testBoolString"], @basicServer.corba_methods
    
    assert_equal "Retn String", @basicServer.foo
    @basicServer.foo = "In string"
    
    assert_equal LONG_RETN, @basicServer.bah

    _in = "In string"
    inout = "Inout in string";
    out = ""
    assert_equal "Retn String", @basicServer.opString(_in, inout, out)
    assert_equal "Inout out string", inout
    assert_equal "Out string", out
    
    _in = ORBit2::Long.new LONG_IN
    inout = ORBit2::Long.new LONG_INOUT_IN
    out = ORBit2::Long.new 0
    assert_equal LONG_RETN, @basicServer.opLong(_in, inout, out)
    assert_equal LONG_INOUT_OUT, inout.to_i
    assert_equal LONG_OUT, out.to_i

    _in = 127.13534
    inout = 124.89432
    out = 0.0
    assert_equal 354.23535, ((@basicServer.opFloat(_in, inout, out)*100000).floor*1.0)/100000
    assert_equal 975.12695, ((inout*100000).floor*1.0)/100000
    assert_equal 112.54575, ((out*100000).floor*1.0)/100000


    _in = 127.13534
    inout = 124.89432
    out = 0.0
    assert_equal 354.23535, @basicServer.opDouble(_in, inout, out)
    assert_equal 975.12694, inout
    assert_equal 112.54575, out

    _in = 0
    inout = ORBit2::Long.new 1
    out = ORBit2::Long.new 2
    assert_equal 4, @basicServer.opEnum(_in, inout, out)
    assert_equal 2, inout.to_i
    assert_equal 3, out.to_i
    
    assert_raises(Orbit::Test::TestException) do
      @basicServer.opException
    end
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
  
  def test_struct(struct_server = nil)
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @structServer = struct_server || @server.getStructServer
    assert_equal ["opFixed", "opVariable", "opCompound", "opAlignHole", "opObjectStruct", "opStructAny"], @structServer.corba_methods
    _in = FixedLengthStruct.new(0x1234)
    inout = FixedLengthStruct.new(ORBit2::Long.new(0x3456))
    out = FixedLengthStruct.new(ORBit2::Long.new(0))
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
    
    _in = CompoundStruct.new(VariableLengthStruct.new("In string"))
    inout = CompoundStruct.new(VariableLengthStruct.new("Inout in string"))
    out = CompoundStruct.new(VariableLengthStruct.new(""))
    retn = @structServer.opCompound(_in, inout, out)
    assert_equal "Inout out string", inout.a.a
    assert_equal "Out string", out.a.a
    assert_equal "Retn String", retn.a.a

    _in = AlignHoleStruct.new(AlignHoleInnerStruct.new(127.13534, 0x13), 0x23)
    inout = AlignHoleStruct.new(AlignHoleInnerStruct.new(124.89432, ORBit2::Long.new(0x35)), ORBit2::Long.new(0x45))
    out = AlignHoleStruct.new(AlignHoleInnerStruct.new(0.0, ORBit2::Long.new(0)), ORBit2::Long.new(0))
    retn = @structServer.opAlignHole(_in, inout, out)
    assert_equal AlignHoleStruct.new(AlignHoleInnerStruct.new(354.23535, ORBit2::Long.new(0xBD)), ORBit2::Long.new(0xAC)), retn
    assert_equal AlignHoleStruct.new(AlignHoleInnerStruct.new(975.12694, ORBit2::Long.new(0x57)), ORBit2::Long.new(0x67)), inout
    assert_equal AlignHoleStruct.new(AlignHoleInnerStruct.new(112.54575, ORBit2::Long.new(0x79)), ORBit2::Long.new(0x89)), out
    
    @derivedServer = @server.getDerivedServer
    @structServer.opObjectStruct(ObjectStruct.new(@derivedServer))
    
    retn = @structServer.opStructAny
    assert_equal StructAny.new("In string", 0x12345678), retn
  end
  
  def test_struct_server_ior
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    ior = @server.getStructServerIOR
    @structServer = ORBit2::CorbaObject.from_ior(ior)
    test_struct(@structServer)
  end
  
  def test_sequence
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @sequenceServer = @server.getSequenceServer
    assert_equal ["opStrSeq", "opBoundedStructSeq", "opMassiveSeq", "opAnySeq"], @sequenceServer.corba_methods
    
    _in = ["in1", "in2"]
    inout = ["inout1","inout2"]
    out = []
    retn = @sequenceServer.opStrSeq(_in, inout, out)
    assert_equal ["retn1","retn2"], retn
    assert_equal ["inout21","inout22"], inout
    assert_equal ["out1","out2"], out
    
    _in = [CompoundStruct.new(VariableLengthStruct.new("in1")), CompoundStruct.new(VariableLengthStruct.new("in2"))]
    inout = [CompoundStruct.new(VariableLengthStruct.new("inout1")), CompoundStruct.new(VariableLengthStruct.new("inout2"))]
    out = []
    retn = @sequenceServer.opBoundedStructSeq(_in, inout, out)
    assert_equal [CompoundStruct.new(VariableLengthStruct.new("retn1")), CompoundStruct.new(VariableLengthStruct.new("retn2"))], retn
    assert_equal [CompoundStruct.new(VariableLengthStruct.new("inout21")), CompoundStruct.new(VariableLengthStruct.new("inout22"))], inout
    assert_equal [CompoundStruct.new(VariableLengthStruct.new("out1")), CompoundStruct.new(VariableLengthStruct.new("out2"))], out
    
    retn = @sequenceServer.opMassiveSeq
    assert_equal 400000, retn.length

    retn = @sequenceServer.opAnySeq
    assert_equal 1000, retn.length
  end
  
  def test_union
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @unionServer = @server.getUnionServer
    assert_equal ["opFixed", "opVariable", "opMisc", "opFixedLengthUnionArray"], @unionServer.corba_methods
  end
  
  def test_array
    return unless @server.is_a?("IDL:orbit/test/TestFactory:1.0")
    @arrayServer = @server.getArrayServer
    assert_equal ["opLongArray", "opOctetArray", "opFixedLengthStructArray", "opStrArray", "opAlignHoleStructArray"], @arrayServer.corba_methods
    
    _in = [LONG_IN, LONG_INOUT_IN, 15, 7]
    inout = [LONG_INOUT_OUT, LONG_OUT, 7, 15]
    out = []
    retn = @arrayServer.opLongArray(_in, inout, out)
    assert_equal [LONG_OUT, LONG_RETN,8,9], inout
    assert_equal [LONG_INOUT_IN, LONG_INOUT_OUT,15,7], out
    assert_equal [LONG_RETN, LONG_IN,2,3], retn
    
    _in = [1, 3, 5, 7]
    inout = [1, 15, 8, 0]
    out = []
    retn = @arrayServer.opOctetArray(_in, inout, out)
    assert_equal [73, 128, 173, 15], inout
    assert_equal [2, 7, 9, 255], out
    assert_equal [1, 3, 5, 7], retn

    _in = [1, 3, 5, 7].map{|a| FixedLengthStruct.new(a)}
    inout = [1, 15, 8, 0].map{|a| FixedLengthStruct.new(a)}
    out = []
    retn = @arrayServer.opFixedLengthStructArray(_in, inout, out)
    assert_equal [73, 128, 173, 15].map{|a| FixedLengthStruct.new(a)}, inout
    assert_equal [2, 7, 9, 255].map{|a| FixedLengthStruct.new(a)}, out
    assert_equal [1, 3, 5, 7].map{|a| FixedLengthStruct.new(a)}, retn
    
    _in = SEQ_STRING_IN
    inout = SEQ_STRING_INOUT_IN.map{|a| a.clone}
    out = []
    retn = @arrayServer.opStrArray(_in, inout, out)
    assert_equal SEQ_STRING_INOUT_OUT, inout
    assert_equal SEQ_STRING_OUT, out
    assert_equal SEQ_STRING_RETN, retn

    _in = (0..3).map{|i| AlignHoleStruct.new(AlignHoleInnerStruct.new(SEQ_OCTET_IN[i], SEQ_OCTET_IN[i]), SEQ_OCTET_IN[i]) }
    inout = (0..3).map{|i| AlignHoleStruct.new(AlignHoleInnerStruct.new(SEQ_OCTET_INOUT_IN[i], SEQ_OCTET_INOUT_IN[i]), SEQ_OCTET_INOUT_IN[i]) }
    out = []
    retn = @arrayServer.opAlignHoleStructArray(_in, inout, out)
    assert_equal (0..3).map{|i| AlignHoleStruct.new(AlignHoleInnerStruct.new(SEQ_OCTET_INOUT_OUT[i], SEQ_OCTET_INOUT_OUT[i]), SEQ_OCTET_INOUT_OUT[i]) }, inout
    assert_equal (0..3).map{|i| AlignHoleStruct.new(AlignHoleInnerStruct.new(SEQ_OCTET_OUT[i], SEQ_OCTET_OUT[i]), SEQ_OCTET_OUT[i]) }, out
    assert_equal (0..3).map{|i| AlignHoleStruct.new(AlignHoleInnerStruct.new(SEQ_OCTET_RETN[i], SEQ_OCTET_RETN[i]), SEQ_OCTET_RETN[i]) }, retn
  end

  def test_exception
  end
end