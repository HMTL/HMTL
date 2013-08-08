#Credit to Lachlan Hardy, taken from this blog post originally:
#http://lachstock.com.au/code/mobile-pages-in-sinatra/

helpers do
  def mobile_user_agent_patterns
    [
      /AppleWebKit.*Mobile/,
      /Android.*AppleWebKit/
    ]
  end

  def mobile_request?
    mobile_user_agent_patterns.any?{ |r|
      request.env['HTTP_USER_AGENT'] =~ r
    }
  end
end