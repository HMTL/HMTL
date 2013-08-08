=begin
class Post
  include DataMapper::Resource
  
  property :id, Serial
  property :title, String
  property :posted_at, DateTime
  property :last_updated, DateTime
  property :body, Text 
end
=end