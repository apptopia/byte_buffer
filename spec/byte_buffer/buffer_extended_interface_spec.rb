# encoding: utf-8
require 'spec_helper'

describe ByteBuffer::Buffer, "extended interface" do
  let(:buffer) {described_class.new}

  describe '#read_int' do
    it 'returns the first four bytes interpreted as signed int' do
      buffer.append("\xff\xee\xdd\xcc")
      buffer.read_int(true).should == -1122868
    end
  end

  describe '#read_long' do
    it 'decodes a positive long' do
      buffer = described_class.new("\x00\x00\xca\xfe\xba\xbe\x00\x00")
      buffer.read_long(true).should == 0x0000cafebabe0000
    end

    it 'decodes an unsigned long' do
      buffer = described_class.new("\xff\xee\xdd\xcc\xbb\xaa\x99\x88")
      buffer.read_long(false).should == 0xffeeddccbbaa9988
    end

    it 'decodes a negative long' do
      buffer = described_class.new("\xff\xee\xdd\xcc\xbb\xaa\x99\x88")
      buffer.read_long(true).should == 0xffeeddccbbaa9988 - 0x10000000000000000
    end

    it 'consumes the bytes' do
      buffer = described_class.new("\xca\xfe\xba\xbe\xca\xfe\xba\xbe\xca\xfe\xba\xbe")
      buffer.read_long
      buffer.should eql_bytes("\xca\xfe\xba\xbe")
    end

    it 'raises an error when there is not enough bytes available' do
      b = described_class.new("\xca\xfe\xba\xbe\x00")
      expect { b.read_long }.to raise_error(RangeError)
    end
  end

  describe '#append_int' do
    it 'encodes an int' do
      buffer.append_int(2323234234)
      buffer.should eql_bytes("\x8a\x79\xbd\xba")
    end

    it 'appends to the buffer' do
      buffer << "\xab"
      buffer.append_int(10)
      buffer.should eql_bytes("\xab\x00\x00\x00\x0a")
    end

    it 'returns the buffer' do
      result = buffer.append_int(2323234234)
      result.should equal(buffer)
    end
  end

  describe '#append_short' do
    it 'encodes a short' do
      buffer.append_short(0xabcd)
      buffer.should eql_bytes("\xab\xcd")
    end

    it 'appends to the buffer' do
      buffer << "\xab"
      buffer.append_short(10)
      buffer.should eql_bytes("\xab\x00\x0a")
    end

    it 'returns the buffer' do
      result = buffer.append_short(42)
      result.should equal(buffer)
    end
  end
end
