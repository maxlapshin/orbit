require 'ruby_orbit'
File.open("/Users/max/Sites/ORBit2/ORBit2-2.14.7/test/echo-server.iorfile") do |f|
  @echo = ORBit2::CorbaObject.new(f.read)
end
puts @echo.corba_methods.inspect
anum = 1.0
@echo.echoString "lala", anum
puts "Result: #{anum}"
