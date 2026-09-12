require "fiddle"

library = Fiddle.dlopen(ARGV.fetch(0))
ui_version = Fiddle::Function.new(library["uiVersion"], [], Fiddle::TYPE_VOIDP)
address = ui_version.call
abort "uiVersion() returned NULL" if address.to_i.zero?

version = address.is_a?(Fiddle::Pointer) ? address.to_s : Fiddle::Pointer.new(address).to_s
abort "uiVersion() returned an empty string" if version.empty?
puts version
