module RPM

  # The base class for RPM exceptions.
  class Error < Exception
    attr_reader :rc

    # Raise a new exception. Expects the message as the normal Exception
    # class, and additionally takes the rpmRC that lead to the exception.
    def initialize(rc = nil)
      @rc = rc
    end


    # Returns a modified string representation of this exception suitable for
    # RPM's internal error passing functionality.
    def to_s
      return "ERROR:#{@rc}:#{super}"
    end
  end
end
