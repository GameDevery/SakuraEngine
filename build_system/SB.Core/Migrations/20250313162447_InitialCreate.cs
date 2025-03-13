using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace SB.Core.Migrations
{
    /// <inheritdoc />
    public partial class InitialCreate : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "Depends",
                columns: table => new
                {
                    TargetName = table.Column<string>(type: "TEXT", nullable: false),
                    InputFiles = table.Column<string>(type: "TEXT", nullable: false),
                    InputFileTimes = table.Column<string>(type: "TEXT", nullable: false),
                    InputArgs = table.Column<string>(type: "TEXT", nullable: true),
                    ExternalFileTimes = table.Column<string>(type: "TEXT", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Depends", x => x.TargetName);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Depends");
        }
    }
}
