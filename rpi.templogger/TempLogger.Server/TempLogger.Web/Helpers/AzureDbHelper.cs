using Microsoft.WindowsAzure.Storage;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Microsoft.Azure;
using Microsoft.WindowsAzure.Storage.Table;
using TempLogger.Web.Models;
using System.Threading.Tasks;

namespace TempLogger.Web.Helpers
{
    public class AzureDbHelper
    {
        private readonly CloudStorageAccount _storageAccount;

        public AzureDbHelper()
        {
            _storageAccount = CloudStorageAccount.Parse(CloudConfigurationManager.GetSetting("StorageConnectionString"));

        }

        public async Task SaveAsync(double temp1, double temp2)
        {
            CloudTableClient tableClient = _storageAccount.CreateCloudTableClient();
            CloudTable table = tableClient.GetTableReference("TempData");
            await table.CreateIfNotExistsAsync();

            TempEntity customer1 = new TempEntity();
            customer1.Temp1 = temp1;
            customer1.Temp2 = temp2;

            // Create the TableOperation that inserts the customer entity.
            TableOperation insertOperation = TableOperation.Insert(customer1);

            // Execute the insert operation.
            await table.ExecuteAsync(insertOperation);
        }

        public async Task<IEnumerable<TempEntity>> GetAll()
        {
            CloudTableClient tableClient = _storageAccount.CreateCloudTableClient();
            CloudTable table = tableClient.GetTableReference("TempData");

            // Construct the query operation for all customer entities where PartitionKey="Smith".
            TableQuery<TempEntity> query = new TableQuery<TempEntity>()
                .Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, "entity"));

            // Print the fields for each customer.
            TableContinuationToken token = null;
            var ret = new List<TempEntity>();
            do
            {
                TableQuerySegment<TempEntity> resultSegment = await table.ExecuteQuerySegmentedAsync(query, token);
                token = resultSegment.ContinuationToken;

                ret.AddRange(resultSegment.Results);
                //foreach (TempEntity entity in resultSegment.Results) xx.Add(entity.Timestamp.DateTime);

            } while (token != null);

            return ret.OrderBy(t => t.Timestamp); 
        }

        public async Task<IEnumerable<TempEntity>> GetSince(DateTimeOffset dtFrom)
        {
            CloudTableClient tableClient = _storageAccount.CreateCloudTableClient();
            CloudTable table = tableClient.GetTableReference("TempData");

            TableQuery<TempEntity> query = new TableQuery<TempEntity>()
                .Where(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, dtFrom));

            // Print the fields for each customer.
            TableContinuationToken token = null;
            var ret = new List<TempEntity>();
            do
            {
                TableQuerySegment<TempEntity> resultSegment = await table.ExecuteQuerySegmentedAsync(query, token);
                token = resultSegment.ContinuationToken;

                ret.AddRange(resultSegment.Results);
                //foreach (TempEntity entity in resultSegment.Results) xx.Add(entity.Timestamp.DateTime);

            } while (token != null);

            return ret.OrderBy(t=>t.Timestamp);
        }
    }
}