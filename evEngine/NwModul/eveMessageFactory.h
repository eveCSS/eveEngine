#ifndef EVEMESSAGEFACTORY_H_
#define EVEMESSAGEFACTORY_H_

#include "eveMessage.h"

/**
 * \brief create messages from stream and vice versa
 *
 * the data stream conforms to the ECP-definition
 */
class eveMessageFactory
{
public:
	eveMessageFactory();
	virtual ~eveMessageFactory();
	static eveMessage * getNewMessage(quint16, quint32, QByteArray *);
	static QByteArray * getNewStream(eveMessage *);
	static bool validate(quint16, quint16, quint32);
};

#endif /*EVEMESSAGEFACTORY_H_*/
