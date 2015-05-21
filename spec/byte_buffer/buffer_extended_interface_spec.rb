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
