#!/usr/bin/env ruby
#--
# Copyright 2010 Per Øyvind Karlsen <peroyvind@mandriva.org>
# This program is free software. It may be redistributed and/or modified under
# the terms of the LGPL version 2.1 (or later).
#
# FIXME: Someone with actual ruby skills should really clean up and sanitize
# 	 this! fugliness obvious...
#++

require 'optparse'
require 'rbconfig'

provides = false
requires = false

opts = OptionParser.new("#{$0} <--provides|--requires>")
opts.on("-P", "--provides", "Print provides") do |val|
  provides = true
end
opts.on("-R", "--requires", "Print requires") do |val|
  requires= true
end

rest = opts.permute(ARGV)

if rest.size != 0 or (!provides and !requires) or (provides and requires)
  $stderr.puts "Use either --provides OR --requires"
  $stderr.puts opts
  exit(1)
end

specpatt = Config::CONFIG["rubylibdir"].sub(Config::CONFIG["ruby_version"], ".*/specifications/.*\.gemspec$")
gems = []
ruby_versioned = false
abi_provide = false

for path in $stdin.readlines
  if path.match(specpatt)
    ruby_versioned = true
    gems.push(path.chomp)
  # this is quite ugly and lame, but the assumption made is that if any files
  # found in any of these directories specific to this ruby version, the
  # package is dependent on this specific version.
  # FIXME: only supports current ruby version
  elsif not ruby_versioned
    if path.match(Config::CONFIG["rubylibdir"])
      ruby_versioned = true
      # even more fugly, but we make the assumption that if the package has
      # this file, the package is the current ruby version, and should
      # therefore provide ruby(abi) = version
      if provides and path.match(Config::CONFIG["rubylibdir"] + "/Env.rb")
	abi_provide = true
      end
    elsif path.match(Config::CONFIG["sitelibdir"])
      ruby_versioned = true
    elsif path.match(Config::CONFIG["vendorlibdir"])
      ruby_versioned = true
    end
  end
end

if requires or abi_provide
  print "ruby(abi)"
  if ruby_versioned
    print " = %s\n" % Config::CONFIG["ruby_version"]
  end
end

if gems.length > 0
  require 'rubygems'

  if requires

    module Gem
      class Requirement
        def rpm_dependency_transform(name, version)
          pessimistic = ""
          if version == "> 0.0.0" or version == ">= 0"
            version = ""
          else
            if version[0..1] == "~>"
              pessimistic = "rubygem(%s) < %s\n" % [name, Gem::Version.create(version[3..-1]).bump]
              version = version.gsub(/\~>/, '=>')
            end
            version = version.gsub(/^/, ' ')
          end
          version = "rubygem(%s)%s\n%s" % [name, version, pessimistic]
        end

        def to_rpm(name)
          result = as_list
          return result.map { |version| rpm_dependency_transform(name, version) }
        end

      end
    end
  end

  for gem in gems
    data = File.read(gem)
    spec = eval(data)
    if provides
      print "rubygem(%s) = %s\n" % [spec.name, spec.version]
    end
    if requires
      for d in spec.dependencies
        print d.requirement.to_rpm(d.name) unless d.type != :runtime
      end
      for d in spec.required_rubygems_version.to_rpm("rubygems")
        print d.gsub(/(rubygem\()|(\))/, "")
      end
    end
  end
end
