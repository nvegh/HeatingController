using System;
using System.Linq;
using System.Threading.Tasks;
using System.Web.Mvc;
using TempController.Web.Extensions;
using TempLogger.Web.Helpers;

namespace TempLogger.Web.Controllers
{
    public class HomeController : Controller
    {
        public ActionResult Index()
        {
            return View();
        }

        /// <summary>
        /// /Home/save?temp1=11&temp2=4
        /// </summary>
        /// <param name="temp1"></param>
        /// <param name="temp2"></param>
        /// <returns></returns>
        [HttpPost]
        public async Task<JsonResult> Save(double temp1, double temp2)
        {
            //Todo: Authorize POST 
            var authHeader = Request.Headers.GetValues("XXX");
            var azure = new AzureDbHelper();
            await azure.SaveAsync(temp1, temp2);

            return Json("ok", JsonRequestBehavior.AllowGet);
        }

        [HttpGet]
        public async Task<JsonResult> Get()
        {
            var azure = new AzureDbHelper();

            DateTime dateFrom = DateTime.Now.Hour < 16
                ? DateTime.Now.Date.AddHours(16).AddDays(-1)
                : DateTime.Now.Date.AddHours(16);

            var data = await azure.GetSince(new DateTimeOffset(dateFrom.AddDays(-3)));

            return Json(
                data
                .Select(t=> new {
                t1 = t.Temp1,
                t2 = t.Temp2,
                d = t.Timestamp.LocalDateTime.ToGoogleDateTimeArray() //TimeZone fix: https://kvaes.wordpress.com/2017/01/24/changing-the-timezone-on-your-azure-webapp-app-service-function/
                })
            , JsonRequestBehavior.AllowGet);

        }

        //TODO: download as csv

    }
}