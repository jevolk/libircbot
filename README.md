
# IRC Bot Library
###### C++ event-based IRC client that keeps state.


#### Requirements

* GNU C++ Compiler **4.9** &nbsp; *(tested: 4.9.1)*
	* GNU libstdc++ **4.9**
	* GCC Locales **4.9**
* Boost &nbsp; *(tested: 1.54)*
	* ASIO
    * Tokenizer
    * Lexical Cast
    * Property Tree, JSON Parser
* LevelDB &nbsp; *(tested: 1.17)*
* STLdb adapter (submodule)


#### Compilation

	git submodule update --init stldb/
	make
