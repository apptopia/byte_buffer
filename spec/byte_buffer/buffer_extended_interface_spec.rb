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
end
