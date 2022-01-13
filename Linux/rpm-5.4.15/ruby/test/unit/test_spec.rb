require 'test/unit'
require 'rpm'
require 'fileutils'
require 'tempfile'
require 'pathname'
require 'tmpdir'

class TestSpec < Test::Unit::TestCase
  def setup
    @fixture_path = File.expand_path File.dirname(__FILE__) + '/../fixtures'
    @macros = [
      File.expand_path(File.dirname(__FILE__) + '/../../../macros/macros'),
      File.expand_path(File.dirname(__FILE__) + 
          '/../../../macros/macros.rpmbuild') ]

    @tmpfile = Tempfile.new 'rpmtest'
    @tmpdir = Dir.mktmpdir

    gmc = RPM::Mc.global_context
    gmc.init_macros(@macros.join ':')
    gmc.add '_sourcedir ' + @fixture_path
    gmc.add '_builddir ' + @tmpdir
    gmc.add '_rpmdir ' + @tmpdir
    gmc.add '_srcrpmdir ' + @tmpdir
    gmc.add 'tmpfile ' + @tmpfile.path

    @ts = RPM::Ts.new
    @spec = @ts.parse_spec File.expand_path(@fixture_path + '/mock.spec'),
      # rootURL, recursing
      '/', false,
      # passphrase, cookie, anyarch, force, verify
      '', nil, false, false, true
  end


  def teardown
    @tmpfile.close
    @tmpfile.unlink
    FileUtils.rmtree @tmpdir
  end


  def test_get_sources
    src = @spec.sources
    
    assert src, 'Must return a sources array'
    assert src.length == 3, 'Must return all three defined sources'
    assert src == %w(barsource1 Foo-1.0 foosource1.tar),
      'Must return the exactly defined source names'
  end

  def test_prep
    @spec.build 1, false

    assert File.exists?(@tmpfile), 'Testfile ' + @tmpfile.path + ' must exist'
    
    @tmpfile.rewind
    assert_equal 'prepared', @tmpfile.read.chomp,
      'Testfile must contain string from %prep section in mock.spec'
  end

  def test_build
    @spec.build 1<<0|1<<1, false

    assert File.exists?(@tmpfile), 'Testfile ' + @tmpfile.path + ' must exist'
    
    @tmpfile.rewind
    assert_equal 'built', @tmpfile.read.chomp,
      'Testfile must contain string from %build section in mock.spec'
  end

  def test_install
    @spec.build 1<<0|1<<2, false

    assert File.exists?(@tmpfile), 'Testfile ' + @tmpfile.path + ' must exist'
    
    @tmpfile.rewind
    assert_equal 'installed', @tmpfile.read.chomp,
      'Testfile must contain string from %install section in mock.spec'
  end

  def test_check
    @spec.build 1<<0|1<<3, false

    assert File.exists?(@tmpfile), 'Testfile ' + @tmpfile.path + ' must exist'
    
    @tmpfile.rewind
    assert_equal 'checked', @tmpfile.read.chomp,
      'Testfile must contain string from %check section in mock.spec'
  end

  def test_clean
    @spec.build 1<<4, false
    assert File.exists?('/tmp/Foo-1.0'), 'Build directory' +
      ' must be removed by now.'
  end

  def test_package_sources
    @spec.build 1<<0|1<<1|1<<2|1<<6

    assert File.exists?(@tmpdir + '/Foo-1.0-1.src.rpm'), 
      'SRCRPM must exist after call to package_sources'
  end

  def test_package_binaries
    @spec.build 1<<0|1<<1|1<<2|1<<7

    assert File.exists?(@tmpdir + '/noarch/Foo-1.0-1.noarch.rpm'), 
      'RPM package must exist after call to package_binaries'
  end
end
