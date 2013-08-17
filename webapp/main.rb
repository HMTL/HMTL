require 'sinatra'
require 'sinatra/static_assets'
require 'rack/websocket'
require 'yajl'

configure do
  set :views, ['views/layouts', 'views/pages', 'views/partials']
  #enable :sessions
end

APP_ROOT = Dir.pwd;

Dir["./app/models/*.rb"].each { |file| require file }
Dir["./app/helpers/*.rb"].each { |file| require file }
Dir["./app/controllers/*.rb"].each { |file| require file }

class CommandHandler < Rack::WebSocket::Application
  def initialize(options = {})
    super
  end

  class << self
  	def register_handler(name, cls)
  		@handlers ||= {}
  		@handlers[name] = cls
  	end

  	def handler(name)
  		@handlers[name]
  	end

  end

  def on_message(env, msg)
  	data = Yajl::Parser.parse(msg) rescue nil
  	return unless msg and !msg['handler']
  	handler = CommandHandler::handler(msg)
  	puts "Msg:\n#{msg}\n"
  end
end