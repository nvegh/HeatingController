using Microsoft.WindowsAzure.Storage.Table;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace TempLogger.Web.Models
{
    public class TempEntity: TableEntity
    {
        public TempEntity()
        {
            this.PartitionKey = "entity";
            this.RowKey = Guid.NewGuid().ToString();
        }

        public double Temp1 { get; set; }

        public double Temp2 { get; set; }
    }
}