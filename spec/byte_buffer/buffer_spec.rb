require 'spec_helper'

describe ByteBuffer::Buffer do
  let(:initial_capacity) {ByteBuffer::Buffer::DEFAULT_PREALLOC_SIZE}

  describe "preallocation" do
    it "preallocates internal buffer" do
      buffer = described_class.new
      expect(buffer.capacity).to eq initial_capacity
    end

    it "preallocates internal buffer which is bigger than default size" do
      buffer = described_class.new('', initial_capacity * 2)
      expect(buffer.capacity).to eq initial_capacity * 2
    end

    it "doesn't preallocate internal buffer smaller than default size" do
      buffer = described_class.new('', initial_capacity / 2)
      expect(buffer.capacity).to eq initial_capacity
    end
  end

  describe "growth strategy" do
    it "grows internal buffer exponentially" do
      buffer = described_class.new
      c0 = buffer.capacity

      buffer.append('X' * initial_capacity)
      expect(buffer.capacity).to eq initial_capacity
      buffer.append('Y')
      c1 = buffer.capacity

      expect(c1).to be > c0

      buffer.append('Z' * (c1 - buffer.size + 1))
      c2 = buffer.capacity

      expect(c2 - c1).to be > (c1 - c0)
    end

    it "doesn't grow buffer when writes and reads are balanced" do
      buffer = described_class.new
      c0 = buffer.capacity
      s = 'X' * c0

      buffer.append(s)
      buffer.read(s.bytesize)
      buffer.append(s)
      buffer.read(s.bytesize)

      expect(buffer.capacity).to eq c0
    end
  end
end
