// $Id: 

#ifndef __HOTKEY_HH__
#define __HOTKEY_HH__

#include <SDL/SDL.h>
#include "EventDistributor.hh"

class HotKey : public EventListener
{
	public:
		virtual ~HotKey();
		static HotKey *instance();

		/**
		 * This is just an extra filter for SDL_KEYDOWN events, now
		 * events are only passed for specific keys.
		 * See EventDistributor::registerAsyncListener for more details
		 */
		void registerAsyncHotKey(SDLKey key, EventListener *listener);


		// EventListener
		void signalEvent(SDL_Event &event);

	private:
		HotKey();
		static HotKey *oneInstance;

		int nbListeners;
		std::multimap <SDLKey, EventListener*> map;
		SDL_mutex *mapMutex;	// to lock variable map
};

#endif
