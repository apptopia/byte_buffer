module ByteBuffer
  class Buffer
    alias_method :size, :length
    alias_method :bytesize, :length
  end
end
