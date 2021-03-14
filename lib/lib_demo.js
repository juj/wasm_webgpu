mergeInto(LibraryManager.library, {
	window_resized_callback: function(callback) {
		window.resizeCallback = callback;
	}
});
