#include "fnd/NonCopyMovable.h"

#include "ICommand.h"
#include "IQuery.h"
#include "ITransaction.h"
#include "sqlite3ppext.h"

namespace HomeCompa::DB::Impl::Sqlite {

std::unique_ptr<ICommand> CreateCommandImpl(sqlite3pp::database& db, std::string_view command);
std::unique_ptr<IQuery>   CreateQueryImpl(sqlite3pp::database& db, std::string_view query);

namespace {

class Transaction final : public ITransaction
{
	NON_COPY_MOVABLE(Transaction)

public:
	explicit Transaction(sqlite3pp::database& db)
		: m_db(db)
		, m_transaction(db, false, true)
	{
	}

	~Transaction() override
	{
		if (m_active)
			m_transaction.rollback();
	}

private: // Transaction
	bool Commit() override
	{
		m_active = false;
		return m_transaction.commit() == 0;
	}

	bool Rollback() override
	{
		m_active = false;
		return m_transaction.rollback() == 0;
	}

	std::unique_ptr<ICommand> CreateCommand(const std::string_view command) override
	{
		return CreateCommandImpl(m_db, command);
	}

	std::unique_ptr<IQuery> CreateQuery(const std::string_view query) override
	{
		return CreateQueryImpl(m_db, query);
	}

private:
	sqlite3pp::database&   m_db;
	sqlite3pp::transaction m_transaction;
	bool                   m_active { true };
};

} // namespace

std::unique_ptr<ITransaction> CreateTransactionImpl(sqlite3pp::database& db)
{
	return std::make_unique<Transaction>(db);
}

} // namespace HomeCompa::DB::Impl::Sqlite
