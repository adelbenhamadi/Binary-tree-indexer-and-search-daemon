#pragma once
#include "../config.h"
#include <string.h>


namespace stemmer {


	/// initialize English stemmar
	void	stem_en_init();

	/// stem English word
	void	stem_en(BYTE* pWord);



}


