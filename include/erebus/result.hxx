#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

enum class Result : int32_t
{
    Success = 0,
    Failure = 1,
    Cancelled = 2,
    InvalidArgument = 3,
    DeadlineExceeded = 4,
    NotFound = 5,
    AlreadyExists = 6,
    PermissionDenied = 7,
    Unauthenticated = 8,
    ResourceExhausted = 9,
    FailedPrecondition = 10,
    Aborted = 11,
    OutOfRange = 12,
    Unimplemented = 13,
    Internal = 14,
    Unavailable = 15,
    DataLoss = 16,
    DoNotUse = -1
};


template <typename StreamT>
inline StreamT& operator<<(StreamT& stream, Result code)
{
    switch (code)
    {
    case Result::Success: stream << "Success"; break;
    case Result::Failure: stream << "Failure"; break;
    case Result::Cancelled: stream << "Cancelled"; break;
    case Result::InvalidArgument: stream << "Invalid argument"; break;
    case Result::DeadlineExceeded: stream << "Deadline exceeded"; break;
    case Result::NotFound: stream << "Not found"; break;
    case Result::AlreadyExists: stream << "Already exists"; break;
    case Result::PermissionDenied: stream << "Permission denied"; break;
    case Result::Unauthenticated: stream << "Unauthenticated"; break;
    case Result::ResourceExhausted: stream << "Resource exhausted"; break;
    case Result::FailedPrecondition: stream << "Failed precondition"; break;
    case Result::Aborted: stream << "Aborted"; break;
    case Result::OutOfRange: stream << "Out of range"; break;
    case Result::Unimplemented: stream << "Unimplemented"; break;
    case Result::Internal: stream << "Internal"; break;
    case Result::Unavailable: stream << "Unavailable"; break;
    case Result::DataLoss: stream << "Data loss"; break;
    default: stream << "???"; break;
    }
    return stream;
}

struct ResultFormatter
{
    void operator()(int32_t v, std::ostream& s) { s << static_cast<Result>(v); }
};



} // namespace Er {}
