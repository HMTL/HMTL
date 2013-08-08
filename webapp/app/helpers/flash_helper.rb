helpers do
  #Flash helper based on the one from here:
  #https://github.com/daddz/sinatra-dm-login/blob/master/helpers/sinatra.rb
  #Call in your views like so: 
  #<% if session[:flash] %>
  #  <p><%= show_flash(:flash) %></p>
  #<% end %>
  def show_flash(key)
    if session[key]
      flash = session[key]
      session[key] = false
      flash
    end
  end
end