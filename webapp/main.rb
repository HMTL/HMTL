require 'sinatra'
require 'sinatra/static_assets'
#require 'data_mapper'

#DataMapper.setup(:default, ENV['DATABASE_URL'] || "sqlite3://#{Dir.pwd}/database.db")

configure do
  set :views, ['views/layouts', 'views/pages', 'views/partials']
  #enable :sessions
end

Dir["./app/models/*.rb"].each { |file| require file }
Dir["./app/helpers/*.rb"].each { |file| require file }
Dir["./app/controllers/*.rb"].each { |file| require file }

before "/*" do 
  set :erb, :layout => :layout
end

#DataMapper.auto_upgrade!
