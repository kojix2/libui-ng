lib LibUI
  fun version = uiVersion : UInt8*
end

version = String.new(LibUI.version)
abort "uiVersion() returned an empty string" if version.empty?
puts version
