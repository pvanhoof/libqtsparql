#include "qsparqlsparqlresultlist_p.h"
#include "qsparqlsparqlconnection_p.h"
#include <QtSparql>

SparqlResultList::SparqlResultList() : connection(0)
{
    modelStatus = Null;
    connect(this, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(countChanged()));
    connect(this, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(this, SIGNAL(started()), this, SLOT(onStarted()));
    lastErrorMessage = QLatin1String("");
}

void SparqlResultList::classBegin()
{
}

void SparqlResultList::componentComplete()
{
    modelStatus = Loading;
    Q_EMIT statusChanged(modelStatus);
    // we will create the connection once the component has finished reading, that way
    // we know if any connection options have been set
}

void SparqlResultList::writeQuery(QString query)
{
    queryString = query;
}

QString SparqlResultList::readQuery() const
{
    return queryString;
}

QVariant SparqlResultList::get(int rowNumber)
{
    QVariantMap map;
    QSparqlResultRow row = resultRow(rowNumber);
    for (int i=0; i<row.count(); i++) {
        map.insert(row.binding(i).name(), row.value(i));
    }
    return map;
}

void SparqlResultList::reload()
{
    setQueryQML(QSparqlQuery(queryString), *connection);
}

void SparqlResultList::setConnection(SparqlConnection* connection)
{
    if (connection)
    {
        this->connection = connection;
        connect(connection, SIGNAL(onCompleted()), this, SLOT(onConnectionComplete()));
    }
}

void SparqlResultList::onConnectionComplete()
{
    if (connection && connection->isValid()) {
        setQueryQML(QSparqlQuery(queryString), *connection);
    } else {
        lastErrorMessage = QLatin1String("Error opening connection");
        modelStatus = Error;
        Q_EMIT statusChanged(modelStatus);
    }
}

SparqlConnection* SparqlResultList::getConnection()
{
    return connection;
}

SparqlResultList::Status SparqlResultList::status()
{
    return modelStatus;
}

QString SparqlResultList::errorString() const
{
    return lastErrorMessage;
}

void SparqlResultList::onStarted()
{
    modelStatus = Loading;
    Q_EMIT statusChanged(modelStatus);
}

void SparqlResultList::onFinished()
{
    if (lastError().type() == QSparqlError::NoError) {
        modelStatus = Ready;
    } else {
        lastErrorMessage = lastError().message();
        modelStatus = Error;
    }
    Q_EMIT statusChanged(modelStatus);
}
