
begin
  require(File.dirname(__FILE__)+'/ruby_orbit') 
rescue LoadError
  require(File.dirname(__FILE__)+"/../ext/ruby_orbit")
end

module ORBit2
  class CorbaObject
    def self.lookup(type_id)
      match_data = type_id.match(/IDL:(.*):\d+\.\d+/)
      klass_name = match_data ? transform_name(match_data.captures.first) : type_id
      klass = nil
      return klass if klass = eval("#{klass_name} rescue nil")
      Orphan.create(klass_name.split("::"))
    end
    
    def to_s_
      "#<#{self.class}:#{ior[0..10]}>"
    end
    
    def self.lookup!(type_id)
			lookup(type_id) || raise(CorbaError, "Couldn't find Ruby class for type id #{type_id}")
    end
    
    def self.transform_name(klass)
      klass.split("/").map {|name| name.capitalize}.join("::")
    end
  end
  
  class Long
    def to_s
      to_i.to_s
    end
    
    def inspect
      to_i.inspect
    end
  end
end