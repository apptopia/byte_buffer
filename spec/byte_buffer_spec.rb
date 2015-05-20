require 'spec_helper'

describe ByteBuffer do
  it 'has a version number' do
    expect(ByteBuffer::VERSION).not_to be nil
  end
end
