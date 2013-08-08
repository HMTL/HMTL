# Sinatra Boilerplate

Sinatra Boilerplate is a lightweight project skeleton for getting a Sinatra project up and running as quickly as possible.

Once you have cloned the project onto your machine, run:

    bundle install

And then:

    rake --tasks

This will show you some helpful tasks that Sinatra Boilerplate provides. 

After you have done that run:

    ruby main.rb

And browse to localhost:4567. Here you will find more information regarding how files are organized within the project.

### Main Features

* Rake tasks to automatically update JQuery and Twitter Bootstrap (if you don't wish to use these, there is a task to remove them)
* Helpers for partials, flash messages and detecting mobile devices
* Sass/Scss, with a rake task to automatically compile a minified version of your stylesheets.
* Datamapper (see Gemfile and main.rb for instructions on deploying to Heroku)
* Minitest (the rake task will still work if you choose to use TestUnit)


### Planned Improvements

I no longer actively contribute to this project anymore. I switched to Rails and then more recently I have been experimenting with Google App Engine. I will still leave this project here in case anyone finds it useful.

### Issues & Feedback

If you have any problems or you're using this for something let me know!
