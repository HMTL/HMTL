require 'rake/testtask'
require 'open-uri'
require 'fileutils'
require 'zip/zipfilesystem'

Rake::TestTask.new do |t|
  t.libs.push("./")
  t.test_files = FileList['tests/*.rb']
  t.verbose = true
end

desc "Compiles all .scss files in the sass directory and places them in public/css (minified)"
task :sass do
  puts "Compiling and minifying .scss files in sass/"
  scss_files = FileList['sass/*.scss']
  scss_files.each do |file|
    puts "Processing: #{file}"
    `sass #{file}:public/css/#{file[5..-5]}css --style compressed`
  end
end

namespace :update do
  desc "Download the latest version of JQuery (minified)"
  task :jquery do
    open('jquery.js', 'wb') do |file|
      file << open('http://code.jquery.com/jquery-latest.min.js').read
    end
    FileUtils.mv('./jquery.js', './public/js/jquery.js')
    puts "Downloaded the latest JQuery"
  end

  desc "Download the latest version of Twitter Bootstrap (minified)"
  task :bootstrap do
    open('bootstrap.zip', 'wb') do |file|
      file << open('http://twitter.github.com/bootstrap/assets/bootstrap.zip').read
    end

    Zip::ZipFile.open("bootstrap.zip") do |zipfile|
      zipfile.each do |file|
        zipfile.extract(file, "./#{file.name}")
      end
    end

    FileUtils.mv('./bootstrap/css/bootstrap.min.css', './public/css/bootstrap.min.css')
    FileUtils.mv('./bootstrap/img/glyphicons-halflings-white.png', './public/img/glyphicons-halflings-white.png')
    FileUtils.mv('./bootstrap/img/glyphicons-halflings.png', './public/img/glyphicons-halflings.png')
    FileUtils.mv('./bootstrap/js/bootstrap.min.js', './public/js/bootstrap.min.js')
    FileUtils.rm_r('./bootstrap')
    FileUtils.rm('./bootstrap.zip')

    puts "Downloaded the latest Twitter Bootstrap"
  end

  desc "Downloads the most recent versions of JQuery & Twitter Bootstrap"
  task :all do
    Rake::Task["update:jquery"].invoke
    Rake::Task["update:bootstrap"].invoke
  end
end

namespace :remove do
  desc "Remove JQuery from the project"
  task :jquery do
    FileUtils.rm('./public/js/jquery.js')
  end

  desc "Remove Twitter Bootstrap from the project"
  task :bootstrap do
    FileUtils.rm('./public/css/bootstrap.min.css')
    FileUtils.rm('./public/img/glyphicons-halflings-white.png')
    FileUtils.rm('./public/img/glyphicons-halflings.png')
    FileUtils.rm('./public/js/bootstrap.min.js')
  end

  desc "Remove both JQuery & Twitter Bootstrap from the project"
  task :all do
    Rake::Task["remove:jquery"].invoke
    Rake::Task["remove:bootstrap"].invoke
  end
end

desc "List all routes for this application"
task :routes do
  puts `grep '^[get|post|put|delete].*do$' app/controllers/*.rb | sed 's/ do$//'`
end

task :default => ["test"]
