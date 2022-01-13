require 'test/unit'
require 'rpm'
require 'fileutils'

class TestRPMMC < Test::Unit::TestCase
  def setup
    @mc = RPM::Mc.new

    dirname = File.dirname __FILE__
    @macro_files = [ File.expand_path(dirname + '/../../../macros/macros'),
      File.expand_path(dirname + '/../fixtures/macros') ]
  end

  
  def test_add_del_expand
    @mc.add('foo Foo')
    @mc.add('foobar %{foo}Bar')

    assert_equal 'Foo', @mc.expand('%foo')
    assert_equal 'FooBar', @mc.expand('%foobar')

    @mc.del('foobar')
    assert_equal '%foobar', @mc.expand('%foobar')
  end


  def test_list
    @mc.add('foo Foo')
    @mc.add('foobar %{foo}Bar')
    @mc.add('bar(i) Argh %{-i}')
    assert_equal [["bar", "i", "Argh %{-i}"], ["foo", "", "Foo"],
      ["foobar", "", "%{foo}Bar"]], @mc.list
  end


  def test_load_macro_file
    @mc.load_macro_file @macro_files[1], 0
    assert_equal 'Bar', @mc.expand('%foo')
    assert_equal 'FooBar', @mc.expand('%foobar')
  end


  def test_init_macros
    @mc.init_macros(@macro_files.join ':')
    assert_equal 'FooBar', @mc.expand('%foobar')
    assert_equal '/usr', @mc.expand('%_usr')
  end

  def test_global_mc
    # Create a scope to check that the gmc persists.

    begin
      gmc = RPM::Mc.global_context
      assert gmc, 'Must return a macro context'

      gmc.add 'foo Foo'
      gmc.add 'foobar %{foo}Bar'
      assert_equal 'FooBar', gmc.expand('%foobar')
    end

    begin
      gmc2 = RPM::Mc.global_context
      assert_equal 'FooBar', gmc.expand('%foobar')
      gmc.del 'foobar'
      gmc.del 'foo'
    end

    begin
      gmc3 = RPM::Mc.global_context
      assert_equal '%foo', gmc.expand('%foo')
    end
  end
end
