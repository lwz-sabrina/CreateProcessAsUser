namespace DemoService
{
    public class Worker : BackgroundService
    {
        private readonly ILogger<Worker> _logger;

        public Worker(ILogger<Worker> logger)
        {
            _logger = logger;
        }

        protected override Task ExecuteAsync(CancellationToken stoppingToken)
        {
            try
            {
                ClrInput.Process.StartProcessAsCurrentUser(
                    @"C:\Project_Code\sqlynx_win_3.4.0\SQLynx.exe",
                    null,
                    null,
                    true
                );
            }
            catch (Exception ex)
            {
                ClrInput.Process.SendMessageToActiveWindow("´íÎó", ex.ToString());
            }
            return Task.CompletedTask;
        }
    }
}
