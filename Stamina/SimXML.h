/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 */


#ifndef SIMXML_H
#define SIMXML_H

#include <string>
#include "RegEx.h"

//#define EXPORT __export

namespace Stamina {
	class SXML {
	private:
		RegEx preg;
		// Pozycje aktualnego elementu

	public:
		SXML();
		~SXML();
		bool loadFile(const char * fileName);
		bool loadSource(const char * xml);
		std::string getSource();

		std::string getContent(const char * path=0); ///<Zwraca cala zawartosc podanej sciezki
		std::string getNode(const char * path=0); ///<Zwraca cala zawartosc podanej sciezki, razem z elementem
		std::string getText(const char * path=0); ///<Zwraca zawartosc tekstowa z podanej sciezki

		bool prepareNode(const std::string& path , bool full=false); ///<Przygotowuje element do czytania
		/// full - czy ma znale�� r�wnie� koniec elementu
		std::string getAttrib(const char * attrib , const char * attribs = 0); ///<Zwraca wartosc atrybutu w attribs, lub w aktualnie wybranym elemencie...
		std::string getAttribs(const char * path=0); ///< Zwraca atrybuty dla elementu

		void next();   ///< Nastepne wywolanie zostanie uruchomione z miejsca ostatniego trafienia
		void restart(); ///< Od poczatku

		struct NodeWalkInfo {
			std::string path;
			size_t start;
			size_t end;
			enum eState {
				undefined=0,
				opened=1,
				closed=2, // <a/>
				closing=3 // </a>
			} state;
			NodeWalkInfo():state(undefined),start(0),end(0){}
		};
		bool nodeWalk(NodeWalkInfo & nfo);

		// Pozycje "obs�ugiwanego" elementu...
		// Najcz�ciej u�ywane wewn�trznie przez funkcje, ale
		// przydaje si�, �eby zapami�ta� go na moment podczas odczytywania
		// atrybut�w z elementu...
		struct Position {
			size_t start;     // pocz�tek
			size_t start_end; // Koniec otwieraj�cego elementu
			size_t end;     // zamykaj�cy element
			size_t end_end; // koniec zamykaj�cego elementu
			Position();
		};
		Position pos;
		//bool pregPath;  // Czy szukane �cie�ki do regExp?

	};


	std::string encodeEntities(std::string txt);
	std::string decodeEntities(std::string txt);
	std::string __stdcall decodeCallback(class RegEx * p , void * param);
	std::string __stdcall encodeCallback(class RegEx * p , void * param);
};

#endif
