#!/usr/bin/env ruby
#--
# Copyright 2010 Per Øyvind Karlsen <peroyvind@mandriva.org>
# This program is free software. It may be redistributed and/or modified under
# the terms of the LGPL version 2.1 (or later).
#++

require 'optparse'

if ARGV[0] == "build" or ARGV[0] == "install"
  require 'yaml'
  require 'zlib'

  filter = nil
  opts = nil
  keepcache = false
  fixperms = false
  gemdir = nil
  dry_run = false
  files = []
  argv = ARGV[1..-1]
  # Push this into some environment variables as the modified classes doesn't
  # seem to be able to access our global variables.. </lameworkaround>
  ENV['GEM_MODE'] = ARGV[0]
  if ARGV[0] == "build"
    opts = OptionParser.new("#{$0} <--filter PATTERN>")
    opts.on("-f", "--filter PATTERN", "Filter pattern to use for gem files") do |val|
      filter = val
    end
    opts.on("-j", "--jobs JOBS", "Number  of  jobs to run simultaneously.") do |val|
      ENV['jobs'] = "-j"+val
    end
    opts.on("--dry-run", "Only show the files the gem will include") do
      ARGV.delete("--dry-run")
      dry_run = true
    end
  elsif ARGV[0] == "install"
    opts = OptionParser.new("#{$0} <--keep-cache>")
    opts.on("--keep-cache", "Don't delete gem copy from cache") do
      ARGV.delete("--keep-cache")
      keepcache = true
    end
    opts.on("--fix-permissions", "Force standard permissions for files installed") do
      ARGV.delete("--fix-permissions")
      fixperms = true
    end    
    opts.on("-i", "--install-dir GEMDIR", "Gem repository directory") do |val|
      gemdir = val
    end
  end
  while argv.length > 0
    begin
      opts.parse!(argv)
    rescue OptionParser::InvalidOption => e
      e.recover(argv)
    end
    argv.delete_at(0)
  end

  file_data = Zlib::GzipReader.open("metadata.gz")
  header = YAML::load(file_data)
  file_data.close()
  body = header.instance_variable_get :@ivars

  require 'rubygems'
  spec = Gem::Specification.from_yaml(YAML.dump(header))

  if ARGV[0] == "install"
    system("gem %s %s.gem" % [ARGV.join(' '), spec.full_name])
    if !keepcache
      require 'fileutils'
      FileUtils.rm_rf("%s/cache" % gemdir)
    end
    if fixperms
      chmod = "chmod u+r,u+w,g-w,g+r,o+r -R %s" % gemdir
      print "\nFixing permissions:\n\n%s\n" % chmod
      system("%s" % chmod)
      print "\n"
    end
  end

  if body['extensions'].size > 0
    require 'rubygems/ext'
    module Gem::Ext
      class Builder
	def self.make(dest_path, results)
	  make_program = ENV['make']
	  unless make_program then
	    make_program = (/mswin/ =~ RUBY_PLATFORM) ? 'nmake' : 'make'
	  end
	  cmd = make_program
	  if ENV['GEM_MODE'] == "build"
  	    cmd += " %s" % ENV['jobs']
	  elsif ENV['GEM_MODE'] == "install"
	    cmd += " DESTDIR='%s' install" % ENV['DESTDIR']
	  end
	  results << cmd
	  results << `#{cmd} #{redirector}`

	  raise Gem::ExtensionBuildError, "make failed:\n\n#{results}" unless
	  $?.success?
	end
      end
    end

    require 'rubygems/installer'
    module Gem
      class Installer
      	def initialize(spec, options={})
	  @gem_dir = Dir.pwd
      	  @spec = spec
	end
      end
      class ConfigFile
	def really_verbose
	  true
	end
      end
    end

    unless dry_run
      Gem::Installer.new(spec).build_extensions
    else
      for ext in body['extensions']
	files.push(ext[0..ext.rindex("/")-1]+".so")
      end
    end

    body['extensions'].clear()
  end
  if ARGV[0] == "build"
    body['test_files'].clear()

    # We don't want ext/ in require_paths, it will only contain content for
    # building extensions which needs to be installed in sitearchdir anyways..
    idx = 0
    for i in 0..body['require_paths'].size()-1
      if body['require_paths'][idx].match("^ext(/|$)")
	body['require_paths'].delete_at(idx)
      else
	idx += 1
      end
    end

    # We'll get rid of all the files we don't really need to install
    idx = 0
    for i in 0..body['files'].size()-1
      if filter and body['files'][idx].match(filter)
	match = true
      else
	match = false
	for path in body['require_paths']
	  if body['files'][idx].match("^%s/" % path)
	    match = true
	  end
	end
      end
      if !match
	body['files'].delete_at(idx)
      else
	idx += 1
      end
    end

    spec = Gem::Specification.from_yaml(YAML.dump(header))
    unless dry_run
      Gem::Builder.new(spec).build
    else
      files.concat(spec.files)
      print "%s\n" % files.join("\n")
    end
  end
end
