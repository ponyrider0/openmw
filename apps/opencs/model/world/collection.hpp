#ifndef CSM_WOLRD_COLLECTION_H
#define CSM_WOLRD_COLLECTION_H

#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <functional>

#include <QVariant>

#include <components/misc/stringops.hpp>

#include "columnbase.hpp"

#include "collectionbase.hpp"

namespace CSMWorld
{
    /// \brief Access to ID field in records
    template<typename ESXRecordT>
    struct IdAccessor
    {
        std::string& getId (ESXRecordT& record);

        const std::string getId (const ESXRecordT& record) const;
    };

    template<typename ESXRecordT>
    std::string& IdAccessor<ESXRecordT>::getId (ESXRecordT& record)
    {
        return record.mId;
    }

    template<typename ESXRecordT>
    const std::string IdAccessor<ESXRecordT>::getId (const ESXRecordT& record) const
    {
        return record.mId;
    }

    /// \brief Single-type record collection
    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
    class Collection : public CollectionBase
    {
        public:

            typedef ESXRecordT ESXRecord;

        private:

			typedef typename std::list<Record<ESXRecordT> >::iterator RecordIteratorT;
			typedef typename std::list<Record<ESXRecordT> >::const_iterator ConstRecordIteratorT;
			typedef std::pair<int, RecordIteratorT> CacheEntry;
			typedef std::pair<int, ConstRecordIteratorT> CacheConstEntry;

			std::list<Record<ESXRecordT> > mRecords;
			std::map<std::string, int> mIndex;
            std::vector<Column<ESXRecordT> *> mColumns;
			CacheEntry mLastAccessedRecord;
			mutable CacheConstEntry mLastAccessedConstRecord;

            // not implemented
            Collection (const Collection&);
            Collection& operator= (const Collection&);

        protected:

            const std::map<std::string, int>& getIdMap() const;

            const std::list<Record<ESXRecordT> >& getRecords() const;

            bool reorderRowsImp (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

        public:

            Collection();

            virtual ~Collection();

			// use the fastest traversal to reach the Nth Record
			typename std::list<Record<ESXRecordT>>::iterator* cacheLookup(int index);
			typename std::list<Record<ESXRecordT>>::iterator getNthRecordIterator(int index);
			typename std::list<Record<ESXRecordT>>::const_iterator getNthRecordIterator(int index) const;
			Record<ESXRecordT>& getNthRecord(int index);
			const Record<ESXRecordT>& getNthRecord(int index) const;
			void advanceRecordIter(typename std::list<Record<ESXRecordT>>::iterator& recordIter, int index);
			void advanceRecordIter(typename std::list<Record<ESXRecordT>>::const_iterator& recordIter, int index) const;
			int getIndexOfRecord(typename std::list<Record<ESXRecordT>>::iterator& record);

			void add (const ESXRecordT& record);
            ///< Add a new record (modified)

            virtual int getSize() const;

            virtual std::string getId (int index) const;

            virtual int getIndex (const std::string& id) const;

            virtual int getColumns() const;

            virtual QVariant getData (int index, int column) const;

            virtual void setData (int index, int column, const QVariant& data);

            virtual const ColumnBase& getColumn (int column) const;

            virtual void merge();
            ///< Merge modified into base.

            virtual void purge();
            ///< Remove records that are flagged as erased.

            virtual void removeRows (int index, int count) ;

            virtual void appendBlankRecord (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None);
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual void cloneRecord(const std::string& origin,
                                     const std::string& destination,
                                     const UniversalId::Type type);

            virtual int searchId (const std::string& id) const;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            virtual void replace (int index, const RecordBase& record);
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.

            virtual void appendRecord (const RecordBase& record,
                UniversalId::Type type = UniversalId::Type_None);
            ///< If the record type does not match, an exception is thrown.
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual const Record<ESXRecordT>& getRecord (const std::string& id) const;

            virtual const Record<ESXRecordT>& getRecord (int index) const;

            virtual int getAppendIndex (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs
            ///
            /// \param listDeleted include deleted record in the list

            virtual void insertRecord (const RecordBase& record, int index,
                UniversalId::Type type = UniversalId::Type_None);
            ///< Insert record before index.
            ///
            /// If the record type does not match, an exception is thrown.
            ///
            /// If the index is invalid either generally (by being out of range) or for the particular
            /// record, an exception is thrown.

            virtual bool reorderRows (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

            void addColumn (Column<ESXRecordT> *column);

            void setRecord (int index, const Record<ESXRecordT>& record);
            ///< \attention This function must not change the ID.

            NestableColumn *getNestableColumn (int column) const;
    };

	// cache lookup
	template<typename ESXRecordT, typename IdAccessorT>
	typename std::list<Record<ESXRecordT>>::iterator* Collection<ESXRecordT, IdAccessorT>::cacheLookup(int index)
	{
		std::list<Record<ESXRecordT>>::iterator iter;
		bool cacheResult;

		// lookup index in cache
		if (index == mLastAccessedRecord->first)
		{
			cacheResult = true;
		}
		else
		{
			cacheResult = false;
		}

		if (cacheResult == false)
		{
			// cacheMiss
			// do lookup 
			iter ( getNthRecordIterator(index) );
			// add to cache
			mLastAccessedRecord->first = index;
			mLastAccessedRecord->second = &iter;
		}
		else
		{
			// cacheHit
			// move to head and return
			iter = mLastAccessedRecord->second;
		}

		// do read-ahead

		return &iter;

	}


	// use the fastest traversal to reach the Nth Record
	template<typename ESXRecordT, typename IdAccessorT>
	typename std::list<Record<ESXRecordT>>::iterator Collection<ESXRecordT, IdAccessorT>::getNthRecordIterator(int index)
	{
		int i, distanceFromEnd, distanceFromCache;
		std::list<Record<ESXRecordT>>::iterator iter;

		if (index == 0)
		{
			if (mLastAccessedRecord.first == -1)
			{
				mLastAccessedRecord.first = 0;
				mLastAccessedRecord.second = mRecords.begin();
			}
			return mRecords.begin();
		}
		else
		{
			if (mLastAccessedRecord.first == index)
				return mLastAccessedRecord.second;
			else if (mLastAccessedRecord.first != -1)
				distanceFromCache = index - mLastAccessedRecord.first;
			else
				distanceFromCache = index;
		}

		distanceFromEnd = mRecords.size() - index;
		if ( (index != 0) && (abs(distanceFromCache) < index) && (abs(distanceFromCache) < distanceFromEnd) )
		{
			if (distanceFromCache > 0)
				// forward
				for (iter = mLastAccessedRecord.second, i = 0; i < distanceFromCache; i++)
					iter++;
			else
				// backward
				for (iter = mLastAccessedRecord.second, i = 0; i > distanceFromCache; i--)
					iter--;
		}
		else
		{
			if (distanceFromEnd < index) {
				// traverse from end()
				for (iter = mRecords.end(), i = mRecords.size(); i > index; i--)
				{
					iter--;
					if (iter == mRecords.end())
					{
						throw std::runtime_error("cached iterator is corrupted or your code is broken.");
					}
				}
			}
			else
			{
				// traverse from begin()
				for (iter = mRecords.begin(), i = 0; i < index; i++)
				{
					iter++;
					if (iter == mRecords.end())
					{
						throw std::runtime_error("cached iterator is corrupted or your code is broken.");
					}
				}
			}

		}

		mLastAccessedRecord.first = index;
		mLastAccessedRecord.second = iter;
		if (mLastAccessedRecord.second != iter)
		{
			throw std::runtime_error("cache is corrupted.");
		}
		return iter;
	}

	// use the fastest traversal to reach the Nth Record
	template<typename ESXRecordT, typename IdAccessorT>
	typename std::list<Record<ESXRecordT>>::const_iterator Collection<ESXRecordT, IdAccessorT>::getNthRecordIterator(int index) const
	{
		int i, distanceFromEnd, distanceFromCache;
		std::list<Record<ESXRecordT>>::const_iterator iter;

		if (index == 0)
		{
			if (mLastAccessedConstRecord.first == -1)
			{
				mLastAccessedConstRecord.first = 0;
				mLastAccessedConstRecord.second = mRecords.begin();
			}
			return mRecords.begin();
		}
		else
		{
			if (mLastAccessedConstRecord.first == index)
				return mLastAccessedConstRecord.second;
			else if (mLastAccessedConstRecord.first != -1)
				distanceFromCache = index - mLastAccessedConstRecord.first;
			else
				distanceFromCache = index;
		}

		distanceFromEnd = mRecords.size() - index;
		if ((index != 0) && (abs(distanceFromCache) < index) && (abs(distanceFromCache) < distanceFromEnd))
		{
			if (distanceFromCache > 0)
				// forward
				for (iter = mLastAccessedConstRecord.second, i = 0; i < distanceFromCache; i++)
				{
					iter++;
					if (iter == mRecords.end())
					{
						throw std::runtime_error("cached iterator is corrupted or your code is broken.");
					}
				}
			else
				// backward
				for (iter = mLastAccessedConstRecord.second, i = 0; i > distanceFromCache; i--)
				{
					iter--;
					if (iter == mRecords.end())
					{
						throw std::runtime_error("cached iterator is corrupted or your code is broken.");
					}
				}
		}
		else
		{
			if (distanceFromEnd < index) {
				// traverse from end()
				for (iter = mRecords.end(), i = mRecords.size(); i > index; i--)
					iter--;
			}
			else
			{
				// traverse from begin()
				for (iter = mRecords.begin(), i = 0; i < index; i++)
					iter++;
			}

		}

		mLastAccessedConstRecord.first = index;
		mLastAccessedConstRecord.second = iter;
		if (mLastAccessedConstRecord.second != iter)
		{
			throw std::runtime_error("cache is corrupted.");
		}
		return iter;
	}

// use the fastest traversal to reach the Nth Record
	template<typename ESXRecordT, typename IdAccessorT>
	Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getNthRecord(int index)
	{
//		std::list<Record<ESXRecordT>>::iterator iter( getNthRecordIterator(index) );
//		return *iter;
		return *getNthRecordIterator(index);
	}

	// use the fastest traversal to reach the Nth Record
	template<typename ESXRecordT, typename IdAccessorT>
	const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getNthRecord(int index) const
	{
//		std::list<Record<ESXRecordT>>::const_iterator iter(getNthRecordIterator(index));
//		return *iter;
		return *getNthRecordIterator(index);
	}

	// advance mRecord iterator by index positions
	template<typename ESXRecordT, typename IdAccessorT>
	void Collection<ESXRecordT, IdAccessorT>::advanceRecordIter(typename std::list<Record<ESXRecordT>>::iterator& recordIter, int index)
	{
		for (int i = 0; i < index; index++)
		{
			++recordIter;
		}

	}

	// advance mRecord iterator by index positions
	template<typename ESXRecordT, typename IdAccessorT>
	void Collection<ESXRecordT, IdAccessorT>::advanceRecordIter(typename std::list<Record<ESXRecordT>>::const_iterator& recordIter, int index) const
	{
		for (int i = 0; i < index; index++)
		{
			++recordIter;
		}

	}

	// calculate index position of iterator
	template<typename ESXRecordT, typename IdAccessorT>
	int Collection<ESXRecordT, IdAccessorT>::getIndexOfRecord(typename std::list<Record<ESXRecordT>>::iterator& record)
	{
		int counter;
		std::list<Record<ESXRecordT>>::iterator currentRecord = record;
		// walk back to the front of list to calculate position in list
		for (counter = 0; currentRecord != mRecords.begin(); counter++)
			--currentRecord;

		return counter;
	}

    template<typename ESXRecordT, typename IdAccessorT>
    const std::map<std::string, int>& Collection<ESXRecordT, IdAccessorT>::getIdMap() const
    {
        return mIndex;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const std::list<Record<ESXRecordT> >& Collection<ESXRecordT, IdAccessorT>::getRecords() const
    {
        return mRecords;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::reorderRowsImp (int baseIndex,
        const std::vector<int>& newOrder)
    {
		std::list<Record<ESXRecordT>>::iterator currentRecord, nextRecord, newPosition;

        if (!newOrder.empty())
        {
            int size = static_cast<int> (newOrder.size());

            // check that all indices are present
            std::vector<int> test (newOrder);
            std::sort (test.begin(), test.end());
            if (*test.begin()!=0 || *--test.end()!=size-1)
                return false;

            // reorder records
//            std::vector<Record<ESXRecordT> > buffer (size);
			std::vector<bool> isSpliced (newOrder.size(), false);

			nextRecord = getNthRecordIterator(baseIndex);
			for (int i=0; i < size; ++i)
            {
				currentRecord = nextRecord;
				++nextRecord;
				// if this is a previously spliced record, skip to the next
				for (int j=i; isSpliced[newOrder[j]] == true; j++)
				{
					currentRecord = nextRecord;
					++nextRecord;
				}
				newPosition = getNthRecordIterator(newOrder[i]);
				mRecords.splice(newPosition, mRecords, currentRecord);
				(*currentRecord).setModified( (*currentRecord).get() );

				// set this new position as spliced so it can be skipped
				isSpliced[newOrder[i]] = true;
//                buffer[newOrder[i]] = mRecords [baseIndex+i];
//                buffer[newOrder[i]].setModified (buffer[newOrder[i]].get());
            }

//            std::copy (buffer.begin(), buffer.end(), mRecords.begin()+baseIndex);

            // update affected mIndex entries
			std::map<std::string, int>::iterator iter;
            for (iter = mIndex.begin(); iter != mIndex.end(); ++iter)
                if ((iter->second >= baseIndex) && (iter->second < baseIndex+size))
                    iter->second = newOrder[iter->second - baseIndex] + baseIndex;
        }

        return true;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::cloneRecord(const std::string& origin,
                                                          const std::string& destination,
                                                          const UniversalId::Type type)
    {
       Record<ESXRecordT> copy;
       copy.mModified = getRecord(origin).get();
       copy.mState = RecordBase::State_ModifiedOnly;
       copy.get().mId = destination;

	   appendRecord(copy);
//       insertRecord(copy, getAppendIndex(destination, type));
    }

	// CONSTRUCTOR
    template<typename ESXRecordT, typename IdAccessorT>
    Collection<ESXRecordT, IdAccessorT>::Collection()
    {
		mLastAccessedRecord.first = -1;
		mLastAccessedConstRecord.first = -1;
	}

	// DESTRUCTOR
    template<typename ESXRecordT, typename IdAccessorT>
    Collection<ESXRecordT, IdAccessorT>::~Collection()
    {
        for (typename std::vector<Column<ESXRecordT> *>::iterator iter (mColumns.begin()); iter!=mColumns.end(); ++iter)
            delete *iter;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::add (const ESXRecordT& record)
    {
        std::string id = Misc::StringUtils::lowerCase (IdAccessorT().getId (record));

        std::map<std::string, int>::iterator iter = mIndex.find (id);

        if (iter == mIndex.end())
        {
			// ID not found in mIndex
            Record<ESXRecordT> record2;
            record2.mState = Record<ESXRecordT>::State_ModifiedOnly;
            record2.mModified = record;

			appendRecord(record2);
//            insertRecord (record2, getAppendIndex (id));
        }
        else
        {
			// ID found
            getNthRecord(iter->second).setModified(record);
        }

		// update mIndex !!!! -----------------------
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getSize() const
    {
        return mRecords.size();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    std::string Collection<ESXRecordT, IdAccessorT>::getId (int index) const
    {
//        return IdAccessorT().getId (mRecords.at (index).get());
		return IdAccessorT().getId( getNthRecord(index).get());
	}

    template<typename ESXRecordT, typename IdAccessorT>
    int  Collection<ESXRecordT, IdAccessorT>::getIndex (const std::string& id) const
    {
        int index = searchId (id);

        if (index==-1)
            throw std::runtime_error ("invalid ID: " + id);

        return index;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getColumns() const
    {
        return mColumns.size();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    QVariant Collection<ESXRecordT, IdAccessorT>::getData (int index, int column) const
    {
//        return mColumns.at (column)->get (mRecords.at (index));
		return mColumns.at(column)->get(getNthRecord(index));
	}

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::setData (int index, int column, const QVariant& data)
    {
//        return mColumns.at (column)->set (mRecords.at (index), data);
		return mColumns.at(column)->set(getNthRecord(index), data);
	}

    template<typename ESXRecordT, typename IdAccessorT>
    const ColumnBase& Collection<ESXRecordT, IdAccessorT>::getColumn (int column) const
    {
        return *mColumns.at (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    NestableColumn *Collection<ESXRecordT, IdAccessorT>::getNestableColumn (int column) const
    {
        if (column < 0 || column >= static_cast<int>(mColumns.size()))
            throw std::runtime_error("column index out of range");

        return mColumns.at (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::addColumn (Column<ESXRecordT> *column)
    {
        mColumns.push_back (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::merge()
    {
//        for (typename std::vector<Record<ESXRecordT> >::iterator iter (mRecords.begin()); iter!=mRecords.end(); ++iter)
		typename std::list<Record<ESXRecordT> >::iterator iter;
		for (iter = mRecords.begin(); iter != mRecords.end(); ++iter)
            iter->merge();

        purge();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void  Collection<ESXRecordT, IdAccessorT>::purge()
    {
        int i = 0;

        while (i < static_cast<int>(mRecords.size()))
        {
//            if (mRecords[i].isErased())
			if ( getNthRecord(i).isErased())
                removeRows (i, 1);
            else
                ++i;
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::removeRows (int index, int count)
    {
		int i;
		std::list<Record<ESXRecordT>>::iterator startPos = mRecords.begin();
		for (i = 0; i < index; i++)
			startPos++;
		std::list<Record<ESXRecordT>>::iterator stopPos = startPos;
		for (i = 0; i < count; i++)
			stopPos++;

//        typename std::map<std::string, int>::iterator iter = mIndex.begin();
//        while (iter != mIndex.end())
		typename std::map<std::string, int>::iterator iter;
		for (iter = mIndex.begin(); iter != mIndex.end(); iter++) 
		{
			if (iter->second >= index) 
			{
				if (iter->second >= index + count)
					iter->second -= count;
				else 
					mIndex.erase(iter);
			}
		}

    }

    template<typename ESXRecordT, typename IdAccessorT>
    void  Collection<ESXRecordT, IdAccessorT>::appendBlankRecord (const std::string& id,
        UniversalId::Type type)
    {
        ESXRecordT record;
        IdAccessorT().getId (record) = id;
        record.blank();

        Record<ESXRecordT> record2;
        record2.mState = Record<ESXRecordT>::State_ModifiedOnly;
        record2.mModified = record;

		appendRecord(record2);
//        insertRecord (record2, getAppendIndex (id, type), type);

		// mIndex !!!!!-----------------------------------

    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::searchId (const std::string& id) const
    {
        std::string id2 = Misc::StringUtils::lowerCase(id);

        std::map<std::string, int>::const_iterator iter = mIndex.find (id2);

        if (iter==mIndex.end())
            return -1;

        return iter->second;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::replace (int index, const RecordBase& record)
    {
//        mRecords.at (index) = dynamic_cast<const Record<ESXRecordT>&> (record);
		getNthRecord(index) = dynamic_cast<const Record<ESXRecordT>&> (record);
	}

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::appendRecord (const RecordBase& record,
        UniversalId::Type type)
    {
		int index = getAppendIndex(IdAccessorT().getId(dynamic_cast<const Record<ESXRecordT>&> (record).get()), type);

		// if insertion point is not at end of vector, use insertRecord
//		if (index < static_cast<int>(mRecords.size())-1 )
		if (index < static_cast<int>(mRecords.size()) )
		{
			insertRecord(record, index);
			return;
		}

		const Record<ESXRecordT>& record2 = dynamic_cast<const Record<ESXRecordT>&> (record);

		mRecords.push_back(dynamic_cast<const Record<ESXRecordT>&> (record2));

		// ????  use append ?????
		mIndex.insert( std::make_pair(Misc::StringUtils::lowerCase(IdAccessorT().getId(record2.get())), index) );
	}

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getAppendIndex (const std::string& id,
        UniversalId::Type type) const
    {
		// std::string id is currently not used.  I'm assuming intention was to find insertion index following the last record with matching id
        return static_cast<int> (mRecords.size());
    }

    template<typename ESXRecordT, typename IdAccessorT>
    std::vector<std::string> Collection<ESXRecordT, IdAccessorT>::getIds (bool listDeleted) const
    {
        std::vector<std::string> ids;
		typename std::map<std::string, int>::const_iterator iter;
        for (iter = mIndex.begin(); iter != mIndex.end(); ++iter)
        {
//            if (listDeleted || !mRecords[iter->second].isDeleted())
			if (listDeleted || !getNthRecord(iter->second).isDeleted() )
                ids.push_back (IdAccessorT().getId (getNthRecord(iter->second).get()));
        }

        return ids;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getRecord (const std::string& id) const
    {
		int index = searchId(id);
//        return mRecords.at (index);
		return getNthRecord(index);
	}

    template<typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getRecord (int index) const
    {
//        return mRecords.at (index);
		return getNthRecord(index);
	}

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::insertRecord (const RecordBase& record, int index,
        UniversalId::Type type)
    {
		if (index<0 || index>static_cast<int> (mRecords.size()))
            throw std::runtime_error ("index out of range");

        const Record<ESXRecordT>& record2 = dynamic_cast<const Record<ESXRecordT>&> (record);
		
		std::list<Record<ESXRecordT> >::iterator recordIter = getNthRecordIterator(index);
        mRecords.insert (recordIter, record2);

		// if index is less than the largest possible index value, update mIndex etnries
		if ( index < static_cast<int>(mRecords.size())-1 )
        {
			std::map<std::string, int>::iterator iter;
            for (iter = mIndex.begin(); iter != mIndex.end(); ++iter)
                 if (iter->second >= index)
                     ++(iter->second);
        }
		// insert new ID-index pair into mIndex
        mIndex.insert ( std::make_pair(Misc::StringUtils::lowerCase(IdAccessorT().getId(record2.get())), index) );
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::setRecord (int index, const Record<ESXRecordT>& record)
    {
		std::string oldRecordmID, newRecordmID;
		oldRecordmID = Misc::StringUtils::lowerCase(IdAccessorT().getId(getNthRecord(index).get()));
		newRecordmID = Misc::StringUtils::lowerCase(IdAccessorT().getId(record.get()));
//        if (Misc::StringUtils::lowerCase (IdAccessorT().getId (mRecords.at (index).get()))!=
//		if (Misc::StringUtils::lowerCase(IdAccessorT().getId(getNthRecord(index).get())) !=
//			Misc::StringUtils::lowerCase (IdAccessorT().getId (record.get())))
		if (oldRecordmID != newRecordmID)
            throw std::runtime_error ("attempt to change the ID of a record");

//        mRecords.at (index) = record;
		getNthRecord(index) = record;
	}

    template<typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::reorderRows (int baseIndex, const std::vector<int>& newOrder)
    {
        return false;
    }
}

#endif
