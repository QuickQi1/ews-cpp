#include "fixture.hpp"
#include <vector>
#include <algorithm>
#include <memory>

// Feature test macro: GNU libstdc++ prior version 4.9.0 does not have
// std::make_unique function
#define EWS_HAS_MAKE_UNIQUE
#if defined(__GLIBCXX__) && __GLIBCXX__ < 20140422
# undef EWS_HAS_MAKE_UNIQUE
#endif

namespace tests
{
    class TaskTest : public BaseFixture
    {
    public:
        virtual ~TaskTest() = default;

        ews::service& service()
        {
            return *service_ptr_;
        }

    private:
        std::unique_ptr<ews::service> service_ptr_;

        virtual void SetUp()
        {
            BaseFixture::SetUp();
            const auto& creds = Environment::credentials();
#ifdef EWS_HAS_MAKE_UNIQUE
            service_ptr_ = std::make_unique<ews::service>(creds.server_uri,
                                                          creds.domain,
                                                          creds.username,
                                                          creds.password);
#else
            service_ptr_ = std::unique_ptr<ews::service>(
                                         new ews::service(creds.server_uri,
                                                          creds.domain,
                                                          creds.username,
                                                          creds.password));
#endif
        }

        virtual void TearDown()
        {
            service_ptr_.reset();
            BaseFixture::TearDown();
        }
    };

    // TODO: test does not need to be in a fixture that can talk to the server
    TEST_F(TaskTest, FromXmlElement)
    {
        using xml_document = rapidxml::xml_document<>;

        // slang: 2013 SP1, not all properties included
        const auto xml = std::string(R"(
            <t:Task
            xmlns:t="http://schemas.microsoft.com/exchange/services/2006/types">
                <t:ItemId Id="abcde" ChangeKey="edcba"/>
                <t:ParentFolderId Id="qwertz" ChangeKey="ztrewq"/>
                <t:ItemClass>IPM.Task</t:ItemClass>
                <t:Subject>Write poem</t:Subject>
                <t:Sensitivity>Normal</t:Sensitivity>
                <t:Body BodyType="Text" IsTruncated="false"/>
                <t:DateTimeReceived>2015-02-09T13:00:11Z</t:DateTimeReceived>
                <t:Size>962</t:Size>
                <t:Importance>Normal</t:Importance>
                <t:IsSubmitted>false</t:IsSubmitted>
                <t:IsDraft>false</t:IsDraft>
                <t:IsFromMe>false</t:IsFromMe>
                <t:IsResend>false</t:IsResend>
                <t:IsUnmodified>false</t:IsUnmodified>
                <t:DateTimeSent>2015-02-09T13:00:11Z</t:DateTimeSent>
                <t:DateTimeCreated>2015-02-09T13:00:11Z</t:DateTimeCreated>
                <t:DisplayCc/>
                <t:DisplayTo/>
                <t:HasAttachments>false</t:HasAttachments>
                <t:Culture>en-US</t:Culture>
                <t:EffectiveRights>
                        <t:CreateAssociated>false</t:CreateAssociated>
                        <t:CreateContents>false</t:CreateContents>
                        <t:CreateHierarchy>false</t:CreateHierarchy>
                        <t:Delete>true</t:Delete>
                        <t:Modify>true</t:Modify>
                        <t:Read>true</t:Read>
                        <t:ViewPrivateItems>true</t:ViewPrivateItems>
                </t:EffectiveRights>
                <t:LastModifiedName>Kwaltz</t:LastModifiedName>
                <t:LastModifiedTime>2015-02-09T13:00:11Z</t:LastModifiedTime>
                <t:IsAssociated>false</t:IsAssociated>
                <t:Flag>
                        <t:FlagStatus>NotFlagged</t:FlagStatus>
                </t:Flag>
                <t:InstanceKey>AQAAAAAAARMBAAAAG4AqWQAAAAA=</t:InstanceKey>
                <t:EntityExtractionResult/>
                <t:ChangeCount>1</t:ChangeCount>
                <t:IsComplete>false</t:IsComplete>
                <t:IsRecurring>false</t:IsRecurring>
                <t:PercentComplete>0</t:PercentComplete>
                <t:Status>NotStarted</t:Status>
                <t:StatusDescription>Not Started</t:StatusDescription>
            </t:Task>)");
        std::vector<char> buf;
        std::copy(begin(xml), end(xml), std::back_inserter(buf));
        buf.push_back('\0');
        xml_document doc;
        doc.parse<0>(&buf[0]);
        auto node = doc.first_node();
        auto t = ews::task::from_xml_element(*node);

        EXPECT_STREQ("Write poem", t.get_subject().c_str());
    }

    TEST_F(TaskTest, CreateTask)
    {
        auto start_time = ews::date_time("2015-01-17T12:00:00Z");
        auto end_time   = ews::date_time("2015-01-17T12:30:00Z");
        auto task = ews::task();
        task.set_subject("Something important to do");
        task.set_body(ews::body("Some descriptive body text"));
        task.set_owner("Donald Duck");
        task.set_start_date(start_time);
        task.set_due_date(end_time);
        task.set_reminder_enabled(true);
        task.set_reminder_due_by(start_time);
        auto item_id = service().create_item(task);

        auto created_task = service().get_task(item_id);
        EXPECT_STREQ("Something important to do",
                     created_task.get_subject().c_str());
    }
}