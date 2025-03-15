#ifndef _COMPARATORS_HXX_
#define _COMPARATORS_HXX_

#include <string>

namespace stucts
{
	struct node
	{
		std::string _word;
		struct node* english;
		struct node* russian;
		struct node* spanish;
		struct node* german;
	};

	struct alphabeticNodeComparator {
		bool operator()(node* a, node* b) const {
			return a->_word < b->_word;
	    	}
	};
}


#endif
