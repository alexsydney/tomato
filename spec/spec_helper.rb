require File.expand_path("../../lib/tomato", __FILE__)

module JavascriptAssertions
  def assert_equal(a, b)
    proc do
      subject.bind_object(b, "obj")
      subject.run <<-end_js
        var a = #{a};
        var b = obj;
        
        if (a != b) {
          if (a && b) { throw " assertion failed: <"+a.toString()+"> is not equal to <"+b.toString()+">"; }
          else if (a) { throw " assertion failed: <"+a.toString()+"> is not equal to <null>"; }
          else if (b) { throw " assertion failed: <null> is not equal to <"+b.toString()+">"; }
          else { throw " assertion failed: <null> is not equal to <null>"; }
        }
        true;
      end_js
    end.should_not raise_error
  end
end

Spec::Runner.configure do |config|
  config.include JavascriptAssertions
end
