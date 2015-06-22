# ByteBuffer

Fast native implementation of binary buffer for MRI.

ByteBuffer is designed to be a drop in replacement of `Ione::ByteBuffer` from [ione gem](https://github.com/iconara/ione). It implements the same interface, extending it with support for additional numeric types and array operations.

It can be used for marshalling/demarshalling of binary frame PDUs. A prime example of such use case is code for handling cassandra native protocol which can be found in `Cassandra::Protocol::CqlNativeByteBuffer` class within [custom branch](https://github.com/apptopia/ruby-driver/tree/perf-tweaks) of cassandra driver gem.

## Benchmarks

As hinted by the description above, main motivation for creating this gem is pursuit of performance. `ByteBuffer::Buffer` is designed to be faster than or even with implementations based on ruby strings and `String#pack`. Here are results of benchmarks taken with ruby 1.9.3 on Mac OS X:

```
  $ ./bin/benchmarks | grep slower
  bm_allocation - Ione::ByteBuffer:  1317389.4 i/s - 1.14x slower
  bm_churn - Ione::ByteBuffer space reuse, read/write interleaved:     6894.2 i/s - 3.32x slower
  bm_append_big - ByteBuffer::Buffer big strings:     7546.2 i/s - 1.03x slower
  bm_append_small - Ione::ByteBuffer small strings:    21482.4 i/s - 2.57x slower
  bm_append_int - Cassandra::Protocol::CqlByteBuffer:    11363.9 i/s - 10.46x slower
  bm_append_long - Cassandra::Protocol::CqlByteBuffer:     5554.0 i/s - 17.85x slower
  bm_append_double - Cassandra::Protocol::CqlByteBuffer:     8569.1 i/s - 11.54x slower
  bm_read - Cassandra::Protocol::CqlByteBuffer:    26853.6 i/s - 2.28x slower
  bm_read_int - Cassandra::Protocol::CqlByteBuffer:    17687.1 i/s - 5.64x slower
  bm_read_long - Cassandra::Protocol::CqlByteBuffer:    12658.3 i/s - 4.32x slower
  bm_read_double - Cassandra::Protocol::CqlByteBuffer:    15554.9 i/s - 3.94x slower
```

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'byte_buffer'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install byte_buffer

## Usage

TODO: Write usage instructions here

## Development

After checking out the repo, install dependencies by running:

```
  $ bundle install
```

Native module can be compiled by running:

```
  $ bundle exec rake compile
```

This should be enough to run tests successfully with:

```
  $ bundle exec rspec
```

Also, there's a script which runs benchmarks against `Ione::ByteBuffer` and `Cassandra::Protocol::CqlNativeByteBuffer`, here's how to run it:

```
  $ ./bin/benchmarks
```
