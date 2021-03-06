/****************************************************************************
**
** Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (ivan.frade@nokia.com)
**
** This file is part of the QtSparql module (not yet part of the Qt Toolkit).
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at ivan.frade@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvariant.h"
#include "qhash.h"
#include "qregexp.h"
#include "qsparqlerror.h"
#include "qsparqlbinding.h"
#include "qsparqlresultrow.h"
#include "qsparqlresult.h"
#include "qvector.h"
#include "qsparqldriver_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

class QSparqlResultPrivate
{
public:
    QSparqlResultPrivate()
    : idx(QSparql::BeforeFirstRow), statementType(QSparqlQuery::SelectStatement),
      boolValue(false)
    {}

public:
    int idx;
    QString sparql;
    QSparqlQuery::StatementType statementType;
    QSparqlError error;
    bool boolValue;
};

/*!
    \class QSparqlResult
    \brief The QSparqlResult class provides an abstract interface for
    accessing the results of an executed QSparqlQuery.

    When QSparqlConnection::exec() is called, it asynchronously begins
    the execution of the given query. The returned result is in an
    unfinished state so that isFinished() returns false. When the execution
    finished, QSparqlResult emits the finished() signal and sets
    itself to finished state.

    Initially, QSparqlResult is positioned on an invalid position. It
    must be navigated to a valid position (so that isValid() returns
    true) before values can be retrieved.

    Navigating the result is performed with the following functions:

    - next()
    - previous()
    - first()
    - last()

    Retrieving the data is performed with the following functions:
    - current()
    - binding()
    - value()
*/

/* this doc not included in doxygen...
    If you are implementing your own SPARQL driver (by subclassing
    QSparqlDriver), you will need to provide your own QSparqlResult
    subclass that implements all the pure virtual functions and other
    virtual functions that you need.

*/

/*!
    Creates a QSparqlResult. The result is empty, positioned at
    "before first row" and unfinished.
*/

QSparqlResult::QSparqlResult()
{
    d = new QSparqlResultPrivate();
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSparqlResult::~QSparqlResult()
{
    delete d;
}

/// Returns the query which was executed for creating this QSparqlResult. Useful
/// for debugging purposes.
QString QSparqlResult::query() const
{
    return d->sparql;
}

/// Returns the statement type of this QSparqlResult object.
QSparqlQuery::StatementType QSparqlResult::statementType() const
{
    return d->statementType;
}

/// Sets the information about the query whose results this QSparqlResult object
/// represents.
void QSparqlResult::setQuery(const QString &query)
{
    d->sparql = query;
}

/// Sets the statement type of this QSparqlResult object.
void QSparqlResult::setStatementType(QSparqlQuery::StatementType type)
{
    d->statementType = type;
}

/// Returns true if this QSparqlResult object represents results that are in the
/// tabular format, e.g., the results of a SELECT query.
bool QSparqlResult::isTable() const
{
    return d->statementType == QSparqlQuery::SelectStatement;
}

/*!
    Returns true if the statement is a CONSTRUCT or DESCRIBE query
    returning a graph. Each QSparqlResultRow in a graph result hasError
    three QSParqlBinding values, named 's', 'p' and 'o' corresponding
    to triples with Subject, Predicate and Object values

    \sa isTable() isBool()
*/

bool QSparqlResult::isGraph() const
{
    return d->statementType == QSparqlQuery::ConstructStatement
            || d->statementType == QSparqlQuery::DescribeStatement;
}

/*!
    Returns true if the statement is an ASK query returning a
    boolean value

    \sa isTable() isGraph() boolValue()
*/

bool QSparqlResult::isBool() const
{
    return d->statementType == QSparqlQuery::AskStatement;
}

/*!
    Returns the boolean result of an ASK query.

    Note that this should only be used when isFinished() is true.

    \sa isBool() setBoolValue()
*/

bool QSparqlResult::boolValue() const
{
    if(!isFinished())
        qWarning() <<
            "QSparqlResult: isFinished() is false, boolValue() may be incorrect";
    return d->boolValue;
}

/*!
    Set the boolean result of an ASK query

    \sa isBool() boolValue()
*/
void QSparqlResult::setBoolValue(bool v)
{
    d->boolValue = v;
}

/*!
    Returns the current internal position of the query. The first
    row is at position zero. If the position is invalid, the
    function returns QSparql::BeforeFirstRow or
    QSparql::AfterLastRow, which are special negative values.

    \sa previous() next() first() last()
*/

int QSparqlResult::pos() const
{
    return d->idx;
}

/*!
    Returns true if the result is positioned on a valid row (that
    is, the result is not positioned before the first or after the
    last row); otherwise returns false.

    \sa pos()
*/

bool QSparqlResult::isValid() const
{
    return d->idx != QSparql::BeforeFirstRow && d->idx != QSparql::AfterLastRow;
}

/*!
    Suspends the execution of the calling thread until all the query results
    have arrived. After this function returns, isFinished() should return true,
    indicating the result's contents are ready to be processed.

    \warning Calling this function from the main thread (the thread that
    calls QApplication::exec()) may cause your user interface to
    freeze.

    \warning Calling this function may cause the events in your event queue to
    be processed.

    \sa isFinished()
*/
void QSparqlResult::waitForFinished()
{
}

/*!
    Returns true if the pending query has finished processing and the result has
    been received. If this function returns true, the hasError() and lastError()
    methods should return valid information.

    The usage of this function differs depending on the driver, and method of
    execution used. For asynchronous queries the results will be available once the
    finished() signal has been emitted, or waitForFinished() has been called. For
    synchronous execution, where the driver supports QSparqlConnection::SyncExec,
    the value of isFinished() will be false until all the results have been retrieved
    using next().

    E.g For a synchronous result
    @verbatim
        QSparqlResult *result = connection.syncExec(query);
        // result->isFinished() will be false
        while(result->next())
            do something
        // result->isFinished() will now be true @endverbatim

    And for asynchronous execution
    @verbatim
        QSparqlResult *result = connection.exec(query);
        result->waitForFinished();
        result->isFinished(); // will be true @endverbatim

    Note that this function only changes state if you call waitForFinished(), or
    if an external event happens, which in general only happens if you return to
    the event loop execution.

    \sa waitForFinished() lastError() error()
*/

bool QSparqlResult::isFinished() const
{
    return false;
}

/*!

  Retrieves the next row in the result, if available, and positions
  the query on the retrieved row. Note that the isTable() or isGraph()
  must return true before calling this function or it will do nothing
  and return false.

  The following rules apply:

  - If the result is currently located before the first row,
  e.g. immediately after a query is executed, an attempt is made to
  retrieve the first row.

  - If the result is currently located after the last row, there
  is no change and false is returned.

  - If the result is located somewhere in the middle, an attempt is
  made to retrieve the next row.

  If the row could not be retrieved, the result is positioned after
  the last row and false is returned. If the row is successfully
  retrieved, true is returned.

  \sa previous() first() last() setPos() pos() isFinished() isValid()
*/
bool QSparqlResult::next()
{
    // qDebug() << "QSparqlResult::next():" << pos() << " size:" << size();

    // Note: Forward only results should re-implement this function, otherwise
    // they cannot work.
    if (hasFeature(QSparqlResult::ForwardOnly)) {
        qWarning() <<
            "QSparqlResult: ForwardOnly QSparqlResult doesn't override next()";
        return false;
    }

    bool b = false;
    int s = size();
    if (s < 0)
        return false;

    switch (pos()) {
    case QSparql::BeforeFirstRow:
        // special case: empty results
        if (s == 0) {
            d->idx = QSparql::AfterLastRow;
            return false;
        }
        b = first();
        return b;
    case QSparql::AfterLastRow:
        return false;
    default:
        if (s < 0 || pos() + 1 < s) {
            return setPos(pos() + 1);
        } else {
            d->idx = QSparql::AfterLastRow;
            return false;
        }
    }
}

/*!

  Retrieves the previous row in the result, if available, and
  positions the query on the retrieved row.

  The following rules apply:

  - If the result is currently located before the first row, there
  is no change and false is returned.

  - If the result is currently located after the last row, an
  attempt is made to retrieve the last row.

  - If the result is somewhere in the middle, an attempt is made to
  retrieve the previous row.

  If the row could not be retrieved, the result is positioned
  before the first row and false is returned. If the row is
  successfully retrieved, true is returned.

  \sa next() first() last() setPos() pos() isFinished() isValid()
*/

bool QSparqlResult::previous()
{
    if (hasFeature(ForwardOnly))  {
        return false;
    }

    bool b = false;
    switch (pos()) {
    case QSparql::BeforeFirstRow:
        return false;
    case QSparql::AfterLastRow:
        // Special case: empty results
        if (size() == 0) {
            d->idx = QSparql::BeforeFirstRow;
            return false;
        }
        b = last();
        return b;
    case 0:
        d->idx = QSparql::BeforeFirstRow;
        return false;
    default:
        return setPos(pos() - 1);
    }
}

/*!
  Retrieves the first row in the result, if available, and
  positions the query on the retrieved row. Returns true if
  successful. If unsuccessful the query position is set to an invalid
  position and false is returned.

  \sa next() previous() last() setPos() pos() isFinished() isValid()
 */

bool QSparqlResult::first()
{
    // Already at the first result
    if (pos() == 0)
        return true;

    if (hasFeature(ForwardOnly)) {
        if (pos() == QSparql::BeforeFirstRow) {
            // if the user hasn't iterated yet, calling first() is the same as
            // calling next() once.
            return next();
        }
        return false;
    }

    return setPos(0);
}

/*!

  Retrieves the last row in the result, if available, and positions the query on
  the retrieved row. Note that the result must be in the finished state before
  calling this function or it will do nothing and return false. Returns true if
  successful. If unsuccessful the query position is set to an invalid position
  and false is returned.

  \sa next() previous() first() setPos() pos() isFinished() isValid()
*/

bool QSparqlResult::last()
{
    // With forward-only results, we don't know which row was the last before we
    // have iterated to it. So, we cannot jump into the last row.
    if (hasFeature(ForwardOnly)) {
        return false;
    }

    int s = size();
    if (s < 0)
        return false;
    return setPos(s - 1);
}

/*!
  Returns the size of the result (number of rows returned).
  
  A return value of -1 is used if the result does not support QuerySize
  information, or if the query has not yet finished (isFinished() returns
  false)

  \sa isFinished() QSparqlResult::hasFeature()
*/

int QSparqlResult::size() const
{
    // The default implementation is OK for ForwardOnly Results. Other Results
    // need to override this function.
    return -1;
}

/*!
    \fn QSparqlBinding QSparqlResult::binding(int index) const

    Returns the binding \a index in the current result row.

    The bindings are numbered from left to right using the text of the
    \c SELECT statement. The indexes start from 0.

    An invalid QSparqlBinding is returned if binding \a index does not
    exist, if the query is inactive, or if the query is positioned on
    an invalid result row.

    \sa value() previous() next() first() last() setPos() isValid()
*/

/*!
    \fn QVariant QSparqlResult::value(int index) const

    Returns the value of binding \a index in the current result row.

    The binding values are numbered from left to right using the text of the
    \c SELECT statement. The indexes start from 0.

    An invalid QVariant is returned if binding value \a index does not
    exist, if the query is inactive, or if the query is positioned on
    an invalid result row.

    \sa binding() previous() next() first() last() setPos() isValid()
*/

/*!
    This function is provided to set the internal (zero-based) row position
    to \a index. If the index is within the range of result rows retrieved
    the function returns true, otherwise false.

    \sa pos()
*/

bool QSparqlResult::setPos(int pos)
{
    if (hasFeature(ForwardOnly)) {
        // For forward-only results, the only legal way to move forward is
        // next(). We cannot say that setPos(pos() + 1) is legal and the same as
        // calling next(), since (it causes weird cornercases when iterating
        // past the end of the result: If the last row is 2, and the user does
        // setPos(3), next() is called, it sets the position to AfterLastRow
        // (and not 3). Should setPos() return true or false? We cannot satisfy
        // these 2 rules: 1) if setPos returns false, it hasn't changed the
        // state of the Result 2) if setPos(i) returns true, pos() returns i.
        return false;
    }

    int s = size();
    if (pos < 0 || (s >= 0 && pos >= s))
        return false;

    d->idx = pos;
    return true;
}

/*!
  Returns the value on column \a column on the current result row as a
  string. This function ignores the type of the data.

  An empty QString is returned if column \a column does not exist, or if the
  result is positioned on an invalid result row.

  \sa binding() value() current()
*/

QString QSparqlResult::stringValue(int i) const
{
    // Subclasses are free to implement this more efficiently
    return value(i).toString();
}

/*!
    This function is provided for derived classes which handle position
    tracking themselves, allowing them to record the current position in the 
    results
*/

void QSparqlResult::updatePos(int index)
{
    // This function dummily udpates d->idx to record the current position. This
    // is used by results which handle the position tracking themselves (e.g.,
    // forward only results use this in their overridden version of next()).
    d->idx = index;
}

/*!
    This function is provided for derived classes to set the last
    error to \a error.

    \sa lastError() hasError()
*/

void QSparqlResult::setLastError(const QSparqlError &error)
{
    d->error = error;
}

/*!
    Returns true if the query has finished and there is an error
    associated with the result.

    \sa setLastError() lastError() isFinished()
*/

bool QSparqlResult::hasError() const
{
    // Don't access d->error unless isFinished is true. This is because a
    // driver-specific thread might set d->error and we have no way to
    // coordinate with it. The driver is responsible for implementing
    // isFinished() in a thread-safe way.
    return isFinished() && d->error.isValid();
}

/*!
    Once the query has finished, returns the last error associated
    with the result

    \sa setLastError() hasError() isFinished()
*/

QSparqlError QSparqlResult::lastError() const
{
    // Don't access d->error unless isFinished is true. This is because a
    // driver-specific thread might set d->error and we have no way to
    // coordinate with it. The driver is responsible for implementing
    // isFinished() in a thread-safe way.
    if (!isFinished())
        return QSparqlError();

    return d->error;
}

/*!
    \enum QSparqlResult::Feature

    This enum contains a list of features a QSparqlResult might support. Use
    hasFeature() to query whether a feature is supported or not. The supported
    features depend on the driver and whether the result was obtained via
    QSparqlConnection::exec() or QSparqlConnection::syncExec().

    \var QSparqlResult::Feature QSparqlResult::QuerySize

    Whether the QSparqlResult can report the number of results of the query.

    \var QSparqlResult::Feature QSparqlResult::ForwardOnly

    Whether the result can only be navigated forward (i.e., using
    QSparqlResult::next()).

    \var QSparqlResult::Feature QSparqlResult::Sync

    Whether the result is natively synchronous (was retrieved via
    QSparqlConnection::syncExec() of natively synchronous connection). In this
    case, QSparqlResult::next() will fetch the next result synchronously.

    \sa hasFeature()
*/

/*!
    Returns true if the QSparqlResult supports feature \a feature;
    otherwise returns false.
*/
bool QSparqlResult::hasFeature(QSparqlResult::Feature) const
{
    return false;
}

/*!
    \fn void QSparqlResult::finished()

    This signal is emitted when the QSparqlResult has finished retrieving its
    data or when there was an error.
*/

/*!
    \fn void QSparqlResult::dataReady(int totalRows)

    This signal is emitted when a query has fetched data. The \a totalRows is
    the row count of the data set after the new data has arrived.
*/

/*!
  \fn QSparqlResultRow QSparqlResult::current() const

  Returns a QSparqlResultRow containing the binding values for the current
  row. If the result points to a valid row (isValid() returns true), the result
  row is populated. An empty result row is returned when there is no result at
  the current position.

  To retrieve just the values from a query, value() should be used since
  its index-based lookup is faster. Use QSparqlResultRow::binding() to
  retrieve the value along with meta data, such as the data type URI
  or language tag for literals.

  \sa value() binding() pos() setPos()
*/

QT_END_NAMESPACE
