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
    let(:buffer) {described_class.new}

    it "fits unsigned int64" do
      buffer.append_long(0xFFFFFFFFFFFFFFFF)
      expect(buffer.read_long).to eq 0xFFFFFFFFFFFFFFFF
    end

    it "fits signed int64" do
      buffer.append_long(-1)
      expect(buffer.read_long).to eq 0xFFFFFFFFFFFFFFFF
      buffer.append_long(- 2**63)
      expect(buffer.read_long).to eq 2**63
    end

    it "fits unsigned int32" do
      buffer.append_int(0xFFFFFFFF)
      expect(buffer.read_int).to eq 0xFFFFFFFF
    end

    it "fits signed int32" do
      buffer.append_int(-1)
      expect(buffer.read_int).to eq 0xFFFFFFFF
      buffer.append_int(- 2**31)
      expect(buffer.read_int).to eq 2**31
    end

    it "fits unsigned int16" do
      buffer.append_short(0xFFFF)
      expect(buffer.read_short).to eq 0xFFFF
    end

    it "fits signed int16" do
      buffer.append_short(-1)
      expect(buffer.read_short).to eq 0xFFFF
      buffer.append_short(- 2**15)
      expect(buffer.read_short).to eq 2**15
    end

    it "fits unsigned int8" do
      buffer.append_byte(0xFF)
      expect(buffer.read_byte).to eq 0xFF
    end

    it "fits signed int8" do
      buffer.append_byte(-1)
      expect(buffer.read_byte).to eq 0xFF
      buffer.append_byte(- 2**7)
      expect(buffer.read_byte).to eq 2**7
    end

    it "raises on overflow of int8" do
      expect(lambda {buffer.append_byte(0xFF + 1)}).to raise_error RangeError
      expect(lambda {buffer.append_byte(-2**7 - 1)}).to raise_error RangeError
      expect(lambda {buffer.append_byte(0xFFFFFFFFFFFFFFFF)}).to raise_error RangeError
      expect(lambda {buffer.append_byte(0xFFFFFFFFFFFFFFFF + 1)}).to raise_error RangeError
    end

    it "raises on overflow of int16" do
      expect(lambda {buffer.append_short(0xFFFF + 1)}).to raise_error RangeError
      expect(lambda {buffer.append_short(-2**15 - 1)}).to raise_error RangeError
    end

    it "raises on overflow of int32" do
      expect(lambda {buffer.append_int(0xFFFFFFFF + 1)}).to raise_error RangeError
      expect(lambda {buffer.append_int(-2**31 - 1)}).to raise_error RangeError
    end

    it "raises on overflow of int64" do
      expect(lambda {buffer.append_long(0xFFFFFFFFFFFFFFFF + 1)}).to raise_error RangeError
      expect(lambda {buffer.append_long(-2**63 - 1)}).to raise_error RangeError
    end

    it "handles signed numbers consistently" do
      buffer.append_byte(-2)
      expect(buffer.read_byte(true)).to eq -2
      buffer.append_byte(- 2**7)
      expect(buffer.read_byte(true)).to eq (- 2**7)

      buffer.append_short(-2)
      expect(buffer.read_short(true)).to eq -2
      buffer.append_short(- 2**15)
      expect(buffer.read_short(true)).to eq (- 2**15)

      buffer.append_int(-2)
      expect(buffer.read_int(true)).to eq -2
      buffer.append_int(- 2**31)
      expect(buffer.read_int(true)).to eq (- 2**31)

      buffer.append_long(- 2**63)
      expect(buffer.read_long(true)).to eq (- 2**63)

      buffer.append_double(-2.5)
      expect(buffer.read_double).to eq -2.5

      buffer.append_float(-2.5)
      expect(buffer.read_float).to eq -2.5
    end
  end

  describe "#append compatibility" do
    class TestBufferSubclass < ByteBuffer::Buffer
    end

    class TestExternalBuffer
      def to_s
        "external buffer content"
      end
    end

    it "accepts descendants of #{described_class}" do
      buffer = described_class.new('ABC')
      buffer.append(TestBufferSubclass.new('DEF'))
      expect(buffer.to_str).to eq 'ABCDEF'
    end

    it "accepts instances of any class via #to_s" do
      buffer = described_class.new('ABC:')
      buffer.append(TestExternalBuffer.new)
      expect(buffer.to_str).to eq 'ABC:external buffer content'
    end
  end
end
