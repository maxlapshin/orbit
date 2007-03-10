#!/usr/bin/env ruby

require 'mkmf'
require 'optparse'

OptionParser.new do |opts|
  opts.on("-O PATH", "--orbit-dir=PATH", "Prefix, where ORBit2 is installed: /usr/local or /opt/local") do |path|
    $LDFLAGS << " -I"+path + "/lib "
    $CFLAGS << " -I" +path + "/include "
  end
  opts.parse!(ARGV.include?("--") ? ARGV[ARGV.index("--")+1..-1] : ARGV.clone)
end

$CFLAGS << " -I/usr/local/include/orbit-2.0 -I/opt/local/include/glib-2.0 -I/opt/local//lib/glib-2.0/include/ "
CONFIG["CC"] = "gcc -Wall -g "

have_header "orbit/orbit.h"
have_library "ORBit-2", "CORBA_ORB_init"
create_makefile "ruby_orbit"