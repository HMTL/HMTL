class DeviceInterface

	def initialize(id)
		@available = false
		after_init if respond_to?(:after_init)
	end

	def send_command(opts = {})

		if !available? 
			return {:status => :error, :type => 'interface_unavailable', :msg => 'Interface unavailable', :opts => opts}
		end

		if !respond_to?(:send)
			return {:status => :error, :type => 'method_not_implemented', :msg => 'Method not implemented', :opts => opts}
		end

		send
	end

	# must implement this 
	def available?
		@available
	end
end

class LightInterface < DeviceInterface

	def after_init

	end


end

class FireInterface < DeviceInterface

end

class CommandRouter

	INTERFACE_MAP_TYPE = {
		:lights => LightInterface,
		:fire => FireInterface
	}

	# pass parsed controls.js file to this
	# creates an interface object for each device that we can send commands to
	def initialize(opts)
		

	end

	def handle_command

	end

end