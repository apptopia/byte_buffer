# ByteBuffer

Fast native implementation of binary buffer for MRI.

ByteBuffer is designed to be a drop in replacement of https://github.com/iconara/ione `Ione::ByteBuffer`. It implements the same interface, extending it with support for additional numeric types and array operations.

It can be used for marshalling/demarshalling of binary frame PDUs. A prime example of such use case is code for handling cassandra native protocol which can be found in `Cassandra::Protocol::CqlNativeByteBuffe` from https://github.com/apptopia/ruby-driver/tree/perf-tweaks

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

After checking out the repo, run `bin/setup` to install dependencies. Then, run `bin/console` for an interactive prompt that will allow you to experiment.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release` to create a git tag for the version, push git commits and tags, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

1. Fork it ( https://github.com/[my-github-username]/byte_buffer/fork )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new Pull Request
