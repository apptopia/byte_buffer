module ByteBuffer
  class Buffer
    alias_method :size, :length
    alias_method :bytesize, :length
    alias_method :<<, :append
    alias_method :cheap_peek, :to_str
    alias_method :to_s, :to_str

    def empty?
      self.length == 0
    end

    def eql?(other)
      self.to_str.eql?(other.to_str)
    end
    alias_method :==, :eql?

    def hash
      to_str.hash
    end

    def dup
      self.class.new(self.to_str)
    end
  end
end
