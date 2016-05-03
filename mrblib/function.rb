module Fiddle
  class Function
    # The ABI of the Function.
    attr_reader :abi

    # The address of this function
    attr_reader :ptr

    # The name of this function
    attr_reader :name

    # The integer memory location of this function
    def to_ptr
      Pointer.new(ptr)
    end
  end
end
