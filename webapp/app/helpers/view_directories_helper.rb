#A slightly hacky solution to get multiple view directories working
#Taken from here: http://www.sinatrarb.com/intro#Looking%20Up%20Template%20Files

helpers do
  def find_template(views, name, engine, &block)
    Array(views).each { |v| super(v, name, engine, &block) }
  end
end