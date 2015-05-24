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

  describe "signs and ranges" do
    it "fits unsigned int64" do
      buffer = described_class.new
      buffer.append_long(0xFFFFFFFFFFFFFFFF)
      expect(buffer.read_long).to eq 0xFFFFFFFFFFFFFFFF
    end

    it "handles signed numbers consistently" do
      buffer = described_class.new

      buffer.append_int(-2)
      expect(buffer.read_int(true)).to eq -2

      buffer.append_long(-2)
      expect(buffer.read_long(true)).to eq -2

      buffer.append_double(-2.5)
      expect(buffer.read_double).to eq -2.5

      buffer.append_float(-2.5)
      expect(buffer.read_float).to eq -2.5
    end
  end

  describe "#append compatibility" do
    it "accepts descendants of #{described_class}"
  end
end
