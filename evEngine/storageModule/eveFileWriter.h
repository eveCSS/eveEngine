/*
 * eveFileWriter.h
 *
 *  Created on: 10.09.2009
 *      Author: eden
 */

#ifndef EVEFILEWRITER_H_
#define EVEFILEWRITER_H_

#include <QHash>
#include <QString>
#include <QStringList>
#include "eveMessage.h"

/*
 *
 */
class eveFileWriter {
public:
	// eveFileWriter();

	virtual ~eveFileWriter() {};
    //! initialize FileWriter plugin. Must be called before all other routines
    /*!
      \param filename name of datafile.
      \param format the datafile format description
      \param parameters a key/value store for plugin parameters
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::init, eveAsciiFileWriter::init
    */
	virtual int init(QString filename, QString format, QHash<QString, QString>& parameters) = 0;
	/*!
	 *
	 * @return a version string to verify library integrity
	 */
	virtual QString getVersionString() = 0;
    //! define a column/dataset, must be called before data may be sent for this column/dataset
    /*!
      \param chainId chainId or Groupname
      \param xmlid xmlId of device (used as datasetname in hdf)
      \param name name of device (used as link to datasetname in hdf)
      \param info List of device properties to be written to file
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::setCols, eveAsciiFileWriter::setCols
    */
//	virtual int setCols(int chainId, QString xmlid, QString name, QStringList info) = 0;
    //! add a column/dataset, must be called before data may be sent for this column/dataset
    /*!
      \param message column / dataset description
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::addColumn, eveAsciiFileWriter::addColumn
    */
	virtual int addColumn(eveDevInfoMessage* message) = 0;
    //! open the file
    /*!
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::open, eveAsciiFileWriter::open
    */
	virtual int open() = 0;
    //! add data to an existing column/dataset
    /*!
      \param chainId chainId or Groupname
      \param message dataMessage containing data to write into file
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::addData, eveAsciiFileWriter::addData
    */
	virtual int addData(int chainId, eveDataMessage* message) = 0;
    //! add metadata to an chain/group
    /*!
      \param chainId chainId or Groupname
      \param attribute attribute name
      \param value attribute value
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::addMetaData, eveAsciiFileWriter::addMetaData
    */
	virtual int addMetaData(int chainId, QString attribute, QString value)=0;
    //! close chain/group
    /*!
      \param chainId chainId or Groupname
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::close, eveAsciiFileWriter::close
    */
	virtual int close() = 0;
    //! store xmldata into file
    /*!
      \param xmldata xml content
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::close, eveAsciiFileWriter::close
    */
	virtual int setXMLData(QByteArray* xmldata) = 0;
	virtual QString errorText() = 0;
    //! flush data to disk
    /*!
      \return status (DEBUG/INFO/MINOR/ERROR/FATAL)
      \sa hdf5Plugin::flush, eveAsciiFileWriter::flush
    */
    virtual int flush() = 0;
};

QT_BEGIN_NAMESPACE

Q_DECLARE_INTERFACE(eveFileWriter,"de.ptb.epics.eve.FileWriterInterface/1.0");

QT_END_NAMESPACE


#endif /* EVEFILEWRITER_H_ */
