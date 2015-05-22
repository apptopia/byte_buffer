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

  describe '#read_signed_int [#read_int(true)]' do
    let :buffer do
      described_class.new("\x00\xff\x00\xff")
    end

    it 'decodes a positive int' do
      buffer.read_int(true).should == 0x00ff00ff
    end

    it 'decodes a negative int' do
      buffer = described_class.new("\xff\xee\xdd\xcc")
      buffer.read_int(true).should == 0xffeeddcc - 0x100000000
    end

    it 'consumes the bytes' do
      buffer << "\xab\xcd"
      buffer.read_int(true)
      buffer.should eql_bytes("\xab\xcd")
    end

    it 'raises an error when there are not enough bytes in the buffer' do
      buffer = described_class.new("\x01\xab")
      expect { buffer.read_int(true) }.to raise_error(RangeError)
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

  describe '#read_double' do
    it 'decodes a double' do
      buffer = described_class.new("@\xC3\x88\x0F\xC2\x7F\x9DU")
      buffer.read_double.should == 10000.123123123
    end

    it 'consumes the bytes' do
      buffer = described_class.new("@\xC3\x88\x0F\xC2\x7F\x9DUxyz")
      buffer.read_double
      buffer.should eql_bytes('xyz')
    end

    it 'raises an error when there is not enough bytes available' do
      buffer = described_class.new("@\xC3\x88\x0F")
      expect { buffer.read_double }.to raise_error(RangeError)
    end
  end

  describe '#read_float' do
    it 'decodes a float' do
      buffer = described_class.new("AB\x14{")
      buffer.read_float.should be_within(0.00001).of(12.13)
    end

    it 'consumes the bytes' do
      buffer = described_class.new("AB\x14{xyz")
      buffer.read_float
      buffer.should eql_bytes('xyz')
    end

    it 'raises an error when there is not enough bytes available' do
      buffer = described_class.new("\x0F")
      expect { buffer.read_float }.to raise_error(RangeError)
    end
  end

  describe '#read_byte_array' do
    it 'decodes unsigned int array' do
      buffer = described_class.new("\x81\x02\x03")
      buffer.read_byte_array(3).should == [129, 2, 3]
    end

    it 'decodes signed int array' do
      buffer = described_class.new("\x81\x02\x03")
      buffer.read_byte_array(3, true).should == [-127, 2, 3]
    end

    it 'consumes bytes' do
      buffer = described_class.new("\x81\x02\x03")
      buffer.read_byte_array(1).should == [129]
      buffer.read_byte_array(2).should == [2, 3]
    end

    it 'raises error when there is not enough bytes available' do
      buffer = described_class.new("\x81\x02\x03")
      buffer.read_byte_array(1)
      expect { buffer.read_byte_array(3) }.to raise_error(RangeError)
    end

    it 'raises error when given negative length' do
      buffer = described_class.new("\x81\x02\x03")
      expect { buffer.read_byte_array(-1) }.to raise_error(RangeError)
    end
  end

  describe '#append_long' do
    it 'encodes a long' do
      buffer.append_long(0x0123456789)
      buffer.should eql_bytes("\x00\x00\x00\x01\x23\x45\x67\x89")
    end

    it 'appends to the buffer' do
      buffer << "\x99"
      buffer.append_long(0x0123456789)
      buffer.should eql_bytes("\x99\x00\x00\x00\x01\x23\x45\x67\x89")
    end

    it 'returns the buffer' do
      result = buffer.append_long(1)
      result.should equal(buffer)
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

  describe '#append_double' do
    it 'encodes a double' do
      buffer.append_double(10000.123123123)
      buffer.should eql_bytes("@\xC3\x88\x0F\xC2\x7F\x9DU")
    end

    it 'appends to the buffer' do
      buffer << 'BEFORE'
      buffer.append_double(10000.123123123)
      buffer.read(6).should eql_bytes('BEFORE')
    end

    it 'returns the buffer' do
      result = buffer.append_double(10000.123123123)
      result.should equal(buffer)
    end
  end

  describe '#append_float' do
    it 'encodes a float' do
      buffer.append_float(12.13)
      buffer.should eql_bytes("AB\x14{")
    end

    it 'appends to the buffer' do
      buffer << 'BEFORE'
      buffer.append_float(12.13)
      buffer.read(6).should eql_bytes('BEFORE')
    end

    it 'returns the buffer' do
      result = buffer.append_float(12.13)
      result.should equal(buffer)
    end
  end
end
