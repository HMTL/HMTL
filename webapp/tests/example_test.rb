require './main'
require 'rack/test'
require 'minitest/autorun'

#Code originally from: https://github.com/neall/boilerplate-sinatra-minitest/blob/master/tests/test_app.rb

ENV['RACK_ENV'] = 'test'

class ExampleTest < MiniTest::Unit::TestCase
  include Rack::Test::Methods

  def app
    Sinatra::Application
  end

  def test_root_path
    get '/'
    assert last_response.ok?
  end
end
