get '/controls.js' do
	content_type 'text/javascript'
	erb :controls, layout: false
end