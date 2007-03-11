
require('ruby_orbit') rescue require(File.dirname(__FILE__)+"/../ext/ruby_orbit")

module ORBit2
  class CorbaObject
    def self.lookup(type_id)
      match_data = type_id.match(/IDL:(.*):\d+\.\d+/)
      raise CorbaError, "Invalid type_id #{type_id}" unless match_data && match_data.captures && match_data.captures.first
      klass_name = transform_name(match_data.captures.first)
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
end