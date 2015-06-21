# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
require "#{lib}/byte_buffer/version"

Gem::Specification.new do |spec|
  spec.name          = "byte_buffer"
  spec.version       = ByteBuffer::VERSION
  spec.authors       = ["Serge Balyuk"]
  spec.email         = ["sergeb@apptopia.com", "bgipsy@gmail.com"]
  spec.homepage      = "http://github.com/apptopia/byte_buffer"
  spec.license       = "MIT"

  spec.summary       = %q{
    Fast native implementation of byte buffer intended to be used as part
    of frame based protocol PDU handling code. Can serve as drop in replacement for Ione::ByteBuffer
    and used in DataStax Cassandra Ruby driver.
  }

  # Prevent pushing this gem to RubyGems.org by setting 'allowed_push_host', or
  # delete this section to allow pushing this gem to any host.
  # if spec.respond_to?(:metadata)
  #   spec.metadata['allowed_push_host'] = "TODO: Set to 'http://mygemserver.com'"
  # else
  #   raise "RubyGems 2.0 or newer is required to protect against public gem pushes."
  # end

  spec.files         = `git ls-files -z`.split("\x0").reject { |f| f.match(%r{^(test|spec|features)/}) }
  spec.bindir        = "exe"
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]
  spec.extensions    = ["ext/byte_buffer_ext/extconf.rb"]

  spec.add_development_dependency "bundler",            "~> 1.9"
  spec.add_development_dependency "rake",               "~> 10.0"
  spec.add_development_dependency "pry",                "~> 0.10.1"
  spec.add_development_dependency "rspec",              "~> 3.2.0"
  spec.add_development_dependency "rake-compiler",      "~> 0.9.5"
  spec.add_development_dependency "benchmark-ips",      "~> 2.2.0"
  spec.add_development_dependency "ione",               "~> 1.2.0"
  spec.add_development_dependency "cassandra-driver",   "~> 2.1.3"
end
