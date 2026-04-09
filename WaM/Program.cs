using System.IO.Ports;
using Spectre.Console;

// --- INITIAL SETUP & PORT SELECTION ---
AnsiConsole.Write(new FigletText("WHACK-A-MOLE").Color(Color.Yellow));

var ports = SerialPort.GetPortNames();
if (ports.Length == 0)
{
    AnsiConsole.MarkupLine("[red]No COM ports found![/] Connect your UNO and restart.");
    return;
}

var selectedPort = AnsiConsole.Prompt(
    new SelectionPrompt<string>()
        .Title("Select your [yellow]Arduino Port[/]:")
        .AddChoices(ports));

using SerialPort sp = new SerialPort(selectedPort, 9600);
int score = 0;
int misses = 0;
int level = 1;
int activeMole = -1;
bool gameOver = false;
bool stopCommandSent = false;
string status = "System Ready. Press 'S' to Start!";

// --- BACKGROUND SERIAL LISTENER ---
sp.DataReceived += (s, e) => {
    try
    {
        string data = sp.ReadLine().Trim();
        if (data.StartsWith("P:")) activeMole = int.Parse(data.Split(':')[1]);
        else if (data.StartsWith("H:")) { score = int.Parse(data.Split(':')[1]); misses = 0; level = GetLevel(score); status = "[green]HIT![/]"; activeMole = -1; }
        else if (data == "M")
        {
            misses++;
            status = "[red]MISS![/]";
            activeMole = -1;

            if (misses >= 5)
            {
                gameOver = true;
                status = "[red]GAME OVER - 5 MISSES![/]";
            }
        }
        else if (data.StartsWith("LOG:"))
        {
            var message = data.Substring(4);
            status = $"[blue]{message}[/]";
            if (message.StartsWith("Game Over"))
            {
                gameOver = true;
                activeMole = -1;
            }
            if (message == "Game Started")
            {
                score = 0;
                misses = 0;
                level = 1;
                gameOver = false;
                stopCommandSent = false;
            }
            if (message == "Score Reset")
            {
                score = 0;
                misses = 0;
                level = 1;
                gameOver = false;
                stopCommandSent = false;
            }
            if (message.StartsWith("Level "))
            {
                level = int.Parse(message.Split(' ')[1]);
            }
        }
    }
    catch { }
};

try { sp.Open(); } catch (Exception ex) { AnsiConsole.MarkupLine($"[red]Error opening port: {ex.Message}[/]"); return; }

// --- UI & INPUT LOOP ---
await AnsiConsole.Live(new Rule())
    .StartAsync(async ctx => {
        while (true)
        {
            if (gameOver && !stopCommandSent)
            {
                sp.Write("X");
                stopCommandSent = true;
            }

            // 1. Handle Keyboard Inputs
            if (Console.KeyAvailable)
            {
                var key = Console.ReadKey(true).Key;
                if (key == ConsoleKey.S) { sp.Write("S"); score = 0; misses = 0; level = 1; gameOver = false; stopCommandSent = false; }
                if (key == ConsoleKey.X) { sp.Write("X"); activeMole = -1; gameOver = true; stopCommandSent = true; }
                if (key == ConsoleKey.R) { sp.Write("R"); score = 0; misses = 0; level = 1; gameOver = false; stopCommandSent = false; }
                if (key == ConsoleKey.Escape) break;
            }

            // 2. Build the UI
            var grid = new Grid().AddColumns(3);
            grid.AddRow(CreateMolePanel(0, activeMole), CreateMolePanel(1, activeMole), CreateMolePanel(2, activeMole));
            grid.AddEmptyRow();
            grid.AddRow(CreateMolePanel(3, activeMole), CreateMolePanel(4, activeMole), CreateMolePanel(5, activeMole));

            var layout = new Table().BorderColor(Color.Blue).Expand()
                .Title($"[bold yellow]PORT: {selectedPort}[/] | [bold cyan]WHACK-A-MOLE[/]")
                .AddColumn(new TableColumn(grid).Centered())
                .AddRow(new Columns(
                    new Panel($"[bold]SCORE:[/] [green]{score}[/]").Expand(),
                    new Panel($"[bold]STATUS:[/] {status}\n[bold]LEVEL:[/] [yellow]{level}[/]\n[bold]MISSES:[/] [red]{misses}/5[/]").Expand()
                ))
                .AddRow(new Panel("[bold white]CONTROLS:[/] [yellow](S)[/] Start  [red](X)[/] Stop  [blue](R)[/] Reset  [grey](Esc)[/] Exit"));

            ctx.UpdateTarget(layout);
            await Task.Delay(50);
        }
    });

Panel CreateMolePanel(int id, int active)
{
    bool isUp = (active == id);
    return new Panel(isUp ? "[bold yellow] (O_O) \n  UP!  [/]" : "[grey] ( - ) \n       [/]")
        .Header($"Hole {id + 1}").BorderColor(isUp ? Color.Yellow : Color.Grey);
}

int GetLevel(int hits)
{
    if (hits >= 20)
    {
        return 3;
    }

    if (hits >= 5)
    {
        return 2;
    }

    return 1;
}