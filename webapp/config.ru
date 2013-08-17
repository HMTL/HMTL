require './main'
run Sinatra::Application

map '/command' do
  run CommandHandler.new
end