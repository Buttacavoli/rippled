//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_PEERTEST_H_INCLUDED
#define RIPPLE_PEERTEST_H_INCLUDED

/** Performs a test of two peers defined by template parameters.
*/
class PeerTest
{
public:
    enum
    {
        /** How long to wait before aborting a peer and reporting a timeout.

            @note Aborting synchronous logics may cause undefined behavior.
        */
        defaultTimeoutSeconds = 30
    };

    //--------------------------------------------------------------------------

    /** Holds the test results for one peer.
    */
    class Result
    {
    public:
        /** Default constructor indicates the test was skipped.
        */
        Result ();

        /** Construct from an error code.
            The prefix is prepended to the error message.
        */
        explicit Result (boost::system::error_code const& ec, String const& prefix = "");

        /** Returns true if the peer failed.
        */
        bool failed () const noexcept;

        /** Convenience for determining if the peer timed out.
        */
        bool timedout () const noexcept;

        /** Provides a descriptive message.
            This is suitable to pass to UnitTest::fail.
        */
        String message () const noexcept;

        /** Report the result to a UnitTest object.
            A return value of true indicates success.
        */
        bool report (UnitTest& test, bool reportPassingTests = false);

    private:
        boost::system::error_code m_ec;
        String m_message;
    };

    //--------------------------------------------------------------------------

    /** Holds the results for both peers in a test.
    */
    struct Results
    {
        String name;        // A descriptive name for this test case.
        Result client;
        Result server;

        Results ();

        /** Report the results to a UnitTest object.
            A return value of true indicates success.
            @param beginTestCase `true` to call test.beginTestCase for you
        */
        bool report (UnitTest& test, bool beginTestCase = true);
    };

    //--------------------------------------------------------------------------

    /** Test two peers and return the results.
    */
    template <typename Details, typename ServerLogic, typename ClientLogic, class Arg>
    static Results run (Arg const& arg, int timeoutSeconds = defaultTimeoutSeconds)
    {
        Results results;

        results.name = Details::getArgName (arg);

        try
        {
            TestPeerType <ServerLogic, Details> server (arg);

            results.name << " / " << server.name ();

            try
            {
                TestPeerType <ClientLogic, Details> client (arg);

                results.name << " / " << client.name ();

                try
                {
                    server.start ();

                    try
                    {
                        client.start ();

                        boost::system::error_code const ec =
                            client.join (timeoutSeconds);

                        results.client = Result (ec, client.name ());

                        try
                        {
                            boost::system::error_code const ec =
                                server.join (timeoutSeconds);

                            results.server = Result (ec, server.name ());

                        }
                        catch (...)
                        {
                            results.server = Result (TestPeerBasics::make_error (
                                TestPeerBasics::errc::exceptioned), server.name ());
                        }
                    }
                    catch (...)
                    {
                        results.client = Result (TestPeerBasics::make_error (
                            TestPeerBasics::errc::exceptioned), client.name ());
                    }
                }
                catch (...)
                {
                    results.server = Result (TestPeerBasics::make_error (
                        TestPeerBasics::errc::exceptioned), server.name ());
                }
            }
            catch (...)
            {
                results.client = Result (TestPeerBasics::make_error (
                    TestPeerBasics::errc::exceptioned), "client");
            }
        }
        catch (...)
        {
            results.server = Result (TestPeerBasics::make_error (
                TestPeerBasics::errc::exceptioned), "server");
        }

        return results;
    }

    //--------------------------------------------------------------------------

    /** Reports tests of Details for all known asynchronous logic combinations to a UnitTest.
    */
    template <typename Details, class Arg>
    static void report_async (UnitTest& test, Arg const& arg,
                              int timeoutSeconds = defaultTimeoutSeconds,
                              bool beginTestCase = true)
    {
        run <Details, TestPeerLogicAsyncServer, TestPeerLogicAsyncClient>
            (arg, timeoutSeconds).report (test, beginTestCase);
    }

    /** Reports tests of Details against all known logic combinations to a UnitTest.
    */
    template <typename Details, class Arg>
    static void report (UnitTest& test, Arg const& arg,
                        int timeoutSeconds = defaultTimeoutSeconds,
                        bool beginTestCase = true)
    {
        run <Details, TestPeerLogicSyncServer,  TestPeerLogicSyncClient>
            (arg, timeoutSeconds).report (test, beginTestCase);
        
        run <Details, TestPeerLogicSyncServer,  TestPeerLogicAsyncClient>
            (arg, timeoutSeconds).report (test, beginTestCase);
        
        run <Details, TestPeerLogicAsyncServer, TestPeerLogicSyncClient>
            (arg, timeoutSeconds).report (test, beginTestCase);

        report_async <Details> (test, arg, timeoutSeconds, beginTestCase);
    }
};

#endif